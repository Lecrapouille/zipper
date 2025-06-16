//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#include "Zipper/Unzipper.hpp"
#include "utils/OS.hpp"
#include "utils/Path.hpp"
#include "utils/glob.hpp"

#include "external/minizip/ioapi_mem.h"
#include "external/minizip/minishared.h"
#include "external/minizip/unzip.h"
#include "external/minizip/zip.h"

#include <array>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

#ifndef ZIPPER_WRITE_BUFFER_SIZE
#    define ZIPPER_WRITE_BUFFER_SIZE (32768u)
#endif

namespace zipper
{

enum class UnzipperError
{
    //! No error
    NO_ERROR_UNZIPPER = 0,
    //! Error when accessing to a info entry
    BAD_ENTRY,
    //! Error when opening a zip file
    OPENING_ERROR,
    //! Error inside this library
    INTERNAL_ERROR,
    //! Zip slip vulnerability, forbidden override files
    SECURITY_ERROR,
    //! Error when extracting a file from a zip file
    EXTRACT_ERROR,
};

// *************************************************************************
//! \brief std::error_code instead of throw() or errno.
// *************************************************************************
struct UnzipperErrorCategory: std::error_category
{
    virtual const char* name() const noexcept override
    {
        return "unzipper";
    }

    virtual std::string message(int /*p_error*/) const override
    {
        return custom_message;
    }

    std::string custom_message;
};

// -----------------------------------------------------------------------------
static UnzipperErrorCategory theUnzipperErrorCategory;

// -----------------------------------------------------------------------------
static std::error_code make_error_code(UnzipperError p_error,
                                       std::string const& p_message)
{
    theUnzipperErrorCategory.custom_message = p_message;
    return { static_cast<int>(p_error), theUnzipperErrorCategory };
}

// *************************************************************************
//! \brief PIMPL implementation
// *************************************************************************
struct Unzipper::Impl
{
    zipFile m_zip_handler = nullptr;
    ourmemory_t m_zip_memory;
    zlib_filefunc_def m_file_func;
    const std::string m_password;
    std::error_code& m_error_code;
    std::vector<char> m_char_buffer;
    std::vector<unsigned char> m_uchar_buffer;
    Unzipper::Progress m_progress;
    Unzipper::ProgressCallback m_progress_callback;

private:

    // -------------------------------------------------------------------------
    bool initMemory(zlib_filefunc_def& p_file_func)
    {
        m_zip_handler = unzOpen2("__notused__", &p_file_func);
        if (m_zip_handler == nullptr)
        {
            m_error_code = make_error_code(UnzipperError::OPENING_ERROR,
                                           OS_STRERROR(errno));
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool locateEntry(std::string const& p_entry_name)
    {
        if (unzLocateFile(m_zip_handler, p_entry_name.c_str(), nullptr) !=
            UNZ_OK)
        {
            std::stringstream str;
            str << "Unknown entry name '"
                << Path::toNativeSeparators(p_entry_name) << "'";
            m_error_code = make_error_code(UnzipperError::BAD_ENTRY, str.str());
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool currentEntryInfo(ZipEntry& p_zip_entry)
    {
        unz_file_info64 file_info;

        // First pass to get the file name size
        int err = unzGetCurrentFileInfo64(
            m_zip_handler, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Invalid zip entry info '"
                << Path::toNativeSeparators(p_zip_entry.name) << "'";
            m_error_code = make_error_code(UnzipperError::BAD_ENTRY, str.str());
            return false;
        }

        // Dynamic allocation of the exact size needed
        uint16_t size_filename =
            static_cast<uint16_t>(file_info.size_filename + 1u);
        std::vector<char> filename_inzip(size_filename, 0);

        // Second pass to get the file name
        err = unzGetCurrentFileInfo64(m_zip_handler,
                                      &file_info,
                                      filename_inzip.data(),
                                      size_filename,
                                      nullptr,
                                      0,
                                      nullptr,
                                      0);
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Invalid zip entry info '"
                << Path::toNativeSeparators(p_zip_entry.name) << "'";
            m_error_code = make_error_code(UnzipperError::BAD_ENTRY, str.str());
            return false;
        }

        p_zip_entry = ZipEntry(std::string(filename_inzip.data()),
                               file_info.compressed_size,
                               file_info.uncompressed_size,
                               file_info.tmu_date.tm_year,
                               file_info.tmu_date.tm_mon,
                               file_info.tmu_date.tm_mday,
                               file_info.tmu_date.tm_hour,
                               file_info.tmu_date.tm_min,
                               file_info.tmu_date.tm_sec,
                               file_info.dos_date);

        // Note: return isEntryValid(entry) is not needed here because invalid
        // entries will be detected later during the extraction.
        return true;
    }

    // -------------------------------------------------------------------------
    inline bool isEntryValid(ZipEntry const& p_zip_entry) const
    {
        auto result = Path::isValidEntry(Path::normalize(p_zip_entry.name));
        std::cout << "isEntryValid: " << p_zip_entry.name << " "
                  << Path::getInvalidEntryReason(result) << std::endl;
        return result == Path::InvalidEntryReason::VALID_ENTRY;
    }

public:

    // -------------------------------------------------------------------------
    std::vector<ZipEntry> entries()
    {
        std::vector<ZipEntry> entries;

        // Count the number of entries and prealloc
        unz_global_info64 gi;
        if (unzGetGlobalInfo64(m_zip_handler, &gi) != UNZ_OK)
        {
            m_error_code = make_error_code(UnzipperError::INTERNAL_ERROR,
                                           "Invalid zip entry info");
            return {};
        }
        entries.reserve(gi.number_entry);

        // Traverse the zip file sequentially in a single pass
        if (unzGoToFirstFile(m_zip_handler) != UNZ_OK)
        {
            m_error_code =
                make_error_code(UnzipperError::INTERNAL_ERROR,
                                "Failed navigating inside zip entries");
            return {};
        }

        int err = UNZ_OK;
        do
        {
            // Get the current entry info
            ZipEntry entry;
            if (currentEntryInfo(entry))
            {
                entries.push_back(entry);
                err = unzGoToNextFile(m_zip_handler);
            }
            else
            {
                // Set the error code if the entry info is invalid
                err = UNZ_ERRNO;
            }
        } while (UNZ_OK == err);

        // If the end of the list of files is not reached and there is
        // an error, return
        if (UNZ_END_OF_LIST_OF_FILE != err && UNZ_OK != err)
        {
            m_error_code =
                make_error_code(UnzipperError::INTERNAL_ERROR,
                                "Failed navigating inside zip entries");
            return {};
        }

        return entries;
    }

    // -------------------------------------------------------------------------
    std::vector<ZipEntry> entries(std::string const& p_glob_pattern)
    {
        std::vector<ZipEntry> all_entries = entries();
        std::vector<ZipEntry> filtered_entries;
        filtered_entries.reserve(all_entries.size());

        for (const auto& entry : all_entries)
        {
            if (p_glob_pattern.empty() ||
                std::regex_match(entry.name, globToRegex(p_glob_pattern)))
            {
                filtered_entries.push_back(entry);
            }
        }

        return filtered_entries;
    }

    // -------------------------------------------------------------------------
    void changeFileDate(std::string const& p_filename,
                        uLong p_dos_date,
                        const tm_zip& p_tmu_date)
    {
#if defined(_WIN32)
        (void)p_tmu_date;
        HANDLE hFile;
        FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

        hFile = CreateFileA(p_filename.c_str(),
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            nullptr,
                            OPEN_EXISTING,
                            0,
                            nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);
            DosDateTimeToFileTime(
                (WORD)(p_dos_date >> 16), (WORD)p_dos_date, &ftLocal);
            LocalFileTimeToFileTime(&ftLocal, &ftm);
            SetFileTime(hFile, &ftm, &ftLastAcc, &ftm);
            CloseHandle(hFile);
        }
#else  // !_WIN32
        (void)p_dos_date;
        struct utimbuf ut;
        struct tm newdate;

        newdate.tm_sec = int(p_tmu_date.tm_sec);
        newdate.tm_min = int(p_tmu_date.tm_min);
        newdate.tm_hour = int(p_tmu_date.tm_hour);
        newdate.tm_mday = int(p_tmu_date.tm_mday);
        newdate.tm_mon = int(p_tmu_date.tm_mon);
        if (p_tmu_date.tm_year > 1900u)
            newdate.tm_year = int(p_tmu_date.tm_year - 1900u);
        else
            newdate.tm_year = int(p_tmu_date.tm_year);
        newdate.tm_isdst = -1;

        ut.actime = ut.modtime = mktime(&newdate);
        utime(p_filename.c_str(), &ut);
#endif // _WIN32
    }

    // -------------------------------------------------------------------------
    int extractToFile(ZipEntry& p_zip_entry,
                      std::string const& p_destination,
                      std::string const& p_canon_output_file,
                      Unzipper::OverwriteMode p_overwrite)
    {
        // Prevent against zip slip attack (See ticket #33)
        if (Path::isZipSlip(p_zip_entry.name, p_destination))
        {
            std::stringstream str;
            str << "Security error: entry '"
                << Path::toNativeSeparators(p_canon_output_file)
                << "' would be outside your target directory";

            m_error_code =
                make_error_code(UnzipperError::EXTRACT_ERROR, str.str());
            return UNZ_ERRNO;
        }
        else if (Path::isValidEntry(p_zip_entry.name) !=
                 Path::InvalidEntryReason::VALID_ENTRY)
        {
            std::stringstream str;
            str << "Security error: entry '"
                << Path::toNativeSeparators(p_canon_output_file) << "' reason: "
                << Path::getInvalidEntryReason(
                       Path::isValidEntry(p_zip_entry.name));

            m_error_code =
                make_error_code(UnzipperError::EXTRACT_ERROR, str.str());
            return UNZ_ERRNO;
        }

        // Check for control characters
        if (Path::checkControlCharacters(p_zip_entry.name) !=
            Path::InvalidEntryReason::VALID_ENTRY)
        {
            std::stringstream str;
            str << "Security error: entry '"
                << Path::toNativeSeparators(p_zip_entry.name)
                << "' contains control characters";
        }

        // Create the folder if the entry is a folder.
        // Note: if (!entryinfo.uncompressed_size) is not a good method to
        // distinguish dummy file from folder. See
        // https://github.com/Lecrapouille/zipper/issues/5
        if (Path::hasTrailingSlash(p_zip_entry.name))
        {
            // Folder name may have an extension file, so we do not add checks
            // if folder name ends with folder slash.
            if (!Path::createDir(p_canon_output_file))
            {
                std::stringstream str;
                str << "Failed creating folder '"
                    << Path::toNativeSeparators(p_canon_output_file) << "'";
                m_error_code =
                    make_error_code(UnzipperError::EXTRACT_ERROR, str.str());
                return UNZ_ERRNO;
            }
            return UNZ_OK;
        }

        // If zip file path contains directories then create them
        std::string folder = Path::dirName(p_canon_output_file);
        if (!folder.empty())
        {
            if (!Path::createDir(folder))
            {
                std::stringstream str;
                str << "Failed creating folder '"
                    << Path::toNativeSeparators(folder)
                    << "'. Reason: " << OS_STRERROR(errno);

                m_error_code =
                    make_error_code(UnzipperError::INTERNAL_ERROR, str.str());
                return UNZ_ERRNO;
            }
        }

        // Avoid replacing the file.
        if ((p_overwrite == Unzipper::OverwriteMode::DoNotOverwrite) &&
            Path::exist(p_canon_output_file))
        {
            std::stringstream str;
            str << "Security Error: '"
                << Path::toNativeSeparators(p_canon_output_file)
                << "' already exists and would have been replaced!";

            m_error_code =
                make_error_code(UnzipperError::SECURITY_ERROR, str.str());
            return UNZ_ERRNO;
        }

        // Create the file on disk so we can unzip to it
        std::ofstream output_file(p_canon_output_file.c_str(),
                                  std::ofstream::binary);
        if (output_file.good())
        {
            int err = extractToStream(output_file, p_zip_entry);
            output_file.close();

            if (err == UNZ_OK)
            {
                // Set the time of the file that has been unzipped
                tm_zip timeaux;
                memcpy(&timeaux, &p_zip_entry.unix_date, sizeof(timeaux));
                changeFileDate(
                    p_canon_output_file.c_str(), p_zip_entry.dos_date, timeaux);
            }

            return err;
        }
        else
        {
            // Possible error is a directory already exists. The errno message
            // is not very explicit.
            std::stringstream str;
            str << "Failed creating file '"
                << Path::toNativeSeparators(p_canon_output_file)
                << "' because: " << OS_STRERROR(errno);

            m_error_code =
                make_error_code(UnzipperError::EXTRACT_ERROR, str.str());
            output_file.close();
            return UNZ_ERRNO;
        }
    }

    // -------------------------------------------------------------------------
    int extractToStream(std::ostream& p_stream, const ZipEntry& p_zip_entry)
    {
        int err;
        int bytes = 0;

        // Open the file with the password. UNZ_OK is returned even if the
        // password is not valid.
        err = unzOpenCurrentFilePassword(
            m_zip_handler, m_password.empty() ? NULL : m_password.c_str());
        if (UNZ_OK == err)
        {
            do
            {
                bytes = unzReadCurrentFile(
                    m_zip_handler,
                    m_char_buffer.data(),
                    static_cast<unsigned int>(m_char_buffer.size()));
                if (bytes <= 0)
                {
                    if (bytes == 0)
                        break; // If password was not valid we reach this case.

                    m_error_code = make_error_code(
                        UnzipperError::INTERNAL_ERROR, OS_STRERROR(errno));
                    return UNZ_ERRNO;
                }

                p_stream.write(m_char_buffer.data(), std::streamsize(bytes));
                if (!p_stream.good())
                {
                    m_error_code = make_error_code(
                        UnzipperError::INTERNAL_ERROR, OS_STRERROR(errno));
                    return UNZ_ERRNO;
                }

                // Update progress
                if (m_progress_callback)
                {
                    m_progress.bytes_read += uint64_t(bytes);
                    m_progress_callback(m_progress);
                }
            } while (bytes > 0);

            p_stream.flush();

            err = unzCloseCurrentFile(m_zip_handler);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Failed closing file '" << p_zip_entry.name << "'";
                m_error_code =
                    make_error_code(UnzipperError::INTERNAL_ERROR, str.str());
            }
        }
        else
        {
            m_error_code =
                make_error_code(UnzipperError::OPENING_ERROR, "Bad password");
        }
        return err;
    }

    // -------------------------------------------------------------------------
    int extractToMemory(std::vector<unsigned char>& p_out_vec,
                        const ZipEntry& p_info)
    {
        int err;
        int bytes = 0;

        err = unzOpenCurrentFilePassword(
            m_zip_handler, m_password.empty() ? NULL : m_password.c_str());
        if (UNZ_OK == err)
        {
            // Pre-allocation to avoid costly reallocations
            p_out_vec.clear();
            p_out_vec.reserve(static_cast<size_t>(p_info.uncompressed_size));

            do
            {
                bytes = unzReadCurrentFile(
                    m_zip_handler,
                    m_uchar_buffer.data(),
                    static_cast<unsigned int>(m_uchar_buffer.size()));
                if (bytes <= 0)
                {
                    if (bytes == 0)
                        break;

                    m_error_code = make_error_code(
                        UnzipperError::INTERNAL_ERROR, OS_STRERROR(errno));
                    return UNZ_ERRNO;
                }

                // Use of insert with iterators for better performance
                p_out_vec.insert(p_out_vec.end(),
                                 m_uchar_buffer.data(),
                                 m_uchar_buffer.data() + bytes);
            } while (bytes > 0);

            err = unzCloseCurrentFile(m_zip_handler);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Failed closing file '" << p_info.name << "'";
                m_error_code =
                    make_error_code(UnzipperError::INTERNAL_ERROR, str.str());
            }
        }
        else
        {
            m_error_code =
                make_error_code(UnzipperError::OPENING_ERROR, "Bad password");
        }

        return err;
    }

public:

    // -------------------------------------------------------------------------
    Impl(std::string const& p_password, std::error_code& p_error_code)
        : m_zip_memory(),
          m_file_func(),
          m_password(p_password),
          m_error_code(p_error_code),
          m_char_buffer(ZIPPER_WRITE_BUFFER_SIZE),
          m_uchar_buffer(ZIPPER_WRITE_BUFFER_SIZE)
    {
        memset(&m_zip_memory, 0, sizeof(m_zip_memory));
        m_zip_handler = nullptr;
    }

    // -------------------------------------------------------------------------
    ~Impl()
    {
        close();
    }

    // -------------------------------------------------------------------------
    void close()
    {
        if (m_zip_handler != nullptr)
        {
            unzClose(m_zip_handler);
            m_zip_handler = nullptr;
        }

        if (m_zip_memory.base != nullptr)
        {
            free(m_zip_memory.base);
            m_zip_memory.base = nullptr;
            m_zip_memory.size = 0;
        }

        // Free the memory of the buffers
        std::vector<char>().swap(m_char_buffer);
        std::vector<unsigned char>().swap(m_uchar_buffer);
    }

    // -------------------------------------------------------------------------
    bool initFile(std::string const& p_filename)
    {
        // Open the zip file
#if defined(_WIN32)
        zlib_filefunc64_def ffunc;
        fill_win32_filefunc64A(&ffunc);
        m_zip_handler = unzOpen2_64(p_filename.c_str(), &ffunc);
#else
        m_zip_handler = unzOpen64(p_filename.c_str());
#endif

        // If the zip file is not opened, return an custom error message
        if (m_zip_handler == nullptr)
        {
            std::stringstream str;
            str << "Failed to open zip file '" << p_filename << "' because: ";

            if (Path::isDir(p_filename))
            {
                str << "Is a directory";
            }
            else if ((errno == EINVAL) ||
                     p_filename.substr(p_filename.find_last_of(".") + 1u) !=
                         "zip")
            {
                str << "Not a zip file";
            }
            else
            {
                str << OS_STRERROR(errno);
            }

            m_error_code =
                make_error_code(UnzipperError::OPENING_ERROR, str.str());
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool initWithStream(std::istream& p_stream)
    {
        // Get the size of the stream
        p_stream.seekg(0, std::ios::end);
        std::streampos s = p_stream.tellg();
        p_stream.seekg(0);

        if (s < 0)
        {
            m_error_code = make_error_code(UnzipperError::INTERNAL_ERROR,
                                           OS_STRERROR(errno));
            return false;
        }
        else
        {
            size_t size = static_cast<size_t>(s);
            m_zip_memory.base = static_cast<char*>(malloc(size * sizeof(char)));
            if (m_zip_memory.base == nullptr)
            {
                m_error_code = make_error_code(UnzipperError::INTERNAL_ERROR,
                                               "Failed to allocate memory");
                return false;
            }
            m_zip_memory.size = static_cast<uint32_t>(size);
            p_stream.read(m_zip_memory.base, std::streamsize(size));
            if (!p_stream.good())
            {
                m_error_code = make_error_code(UnzipperError::INTERNAL_ERROR,
                                               OS_STRERROR(errno));
                return false;
            }
        }

        fill_memory_filefunc(&m_file_func, &m_zip_memory);
        return initMemory(m_file_func);
    }

    // -------------------------------------------------------------------------
    bool initWithVector(const std::vector<unsigned char>& p_buffer)
    {
        // Initialize the zip memory
        m_zip_memory.grow = 1;
        m_zip_memory.size = 0;

        // Free existing memory if any
        if (m_zip_memory.base != nullptr)
        {
            free(m_zip_memory.base);
            m_zip_memory.base = nullptr;
            m_zip_memory.size = 0;
        }

        // Empty vector case is allowed
        if (!p_buffer.empty())
        {
            m_zip_memory.base =
                reinterpret_cast<char*>(malloc(p_buffer.size() * sizeof(char)));
            memcpy(m_zip_memory.base, p_buffer.data(), p_buffer.size());
            m_zip_memory.size = static_cast<uint32_t>(p_buffer.size());
        }

        fill_memory_filefunc(&m_file_func, &m_zip_memory);
        return initMemory(m_file_func);
    }

    // -------------------------------------------------------------------------
    bool
    extractAll(std::string const& p_glob_pattern,
               std::string const& p_destination_folder,
               const std::map<std::string, std::string>& p_alternative_names,
               Unzipper::OverwriteMode p_overwrite)
    {
        m_error_code.clear();
        size_t failures = 0;

        if (m_progress_callback)
        {
            m_progress.reset();

            // Get total number of files and bytes
            std::vector<ZipEntry> _entries = entries();
            for (const auto& entry : _entries)
            {
                // TODO: ICI mettre glob pattern
                m_progress.total_bytes += entry.uncompressed_size;
            }
            m_progress.total_files = _entries.size();
            m_progress_callback(m_progress);
        }

        // Preparation of the destination prefix once.
        // Shall be a folder name, add a separator if not already present.
        std::string destination;
        if (!p_destination_folder.empty())
        {
            destination = Path::folderNameWithSeparator(p_destination_folder);
        }

        // TODO: ici iterer sur _entries et appeller directement extractToFile

        // Traverse the zip file sequentially in a single pass
        int err = unzGoToFirstFile(m_zip_handler);
        if (err != UNZ_OK)
        {
            m_error_code = make_error_code(UnzipperError::INTERNAL_ERROR,
                                           "Failed going to first entry");
            if (m_progress_callback)
            {
                m_progress.status = Progress::Status::KO;
                m_progress_callback(m_progress);
            }
            return false;
        }

        while (err == UNZ_OK)
        {
            ZipEntry entry;
            if (currentEntryInfo(entry))
            {
                // If the entry name matches the glob pattern, extract it else
                // skip it.
                if ((p_glob_pattern.empty()) ||
                    (std::regex_match(entry.name, globToRegex(p_glob_pattern))))
                {
                    // If an alternative name is provided for this entry, use
                    // it, otherwise use the entry name.
                    std::string file_path = destination;
                    auto const& alt = p_alternative_names.find(entry.name);
                    if (alt != p_alternative_names.end())
                        file_path += alt->second;
                    else
                        file_path += entry.name;
                    std::string canon_output_file = Path::normalize(file_path);

                    // Update progress
                    if (m_progress_callback)
                    {
                        m_progress.current_file = canon_output_file;
                        m_progress.status = Progress::Status::InProgress;
                        m_progress_callback(m_progress);
                    }

                    if (extractToFile(entry,
                                      destination,
                                      canon_output_file,
                                      p_overwrite) != UNZ_OK)
                    {
                        failures++;
                        if (m_progress_callback)
                        {
                            m_progress.status = Progress::Status::KO;
                            m_progress_callback(m_progress);
                        }
                    }
                    else if (m_progress_callback)
                    {
                        m_progress.files_extracted++;
                        m_progress.status = Progress::Status::InProgress;
                        m_progress_callback(m_progress);
                    }
                }
            }
            else
            {
                failures++;
                if (m_progress_callback)
                {
                    m_progress.current_file = "???";
                    m_progress.status = Progress::Status::KO;
                    m_progress_callback(m_progress);
                }
            }

            // Go to the next entry.
            err = unzGoToNextFile(m_zip_handler);
        }

        // If the end of the list of files has not been reached
        if (((err != UNZ_END_OF_LIST_OF_FILE) && (err != UNZ_OK)) ||
            (failures > 0u))
        {
            /* TODO
            std::stringstream msg;
            msg << "Failed extracting "
                << (failures == 0u ? "all" : std::to_string(failures))
                << (failures == 1u ? " file" : " files") << std::endl;
            m_error_code =
                make_error_code(UnzipperError::EXTRACT_ERROR, msg.str());
                */
            if (m_progress_callback)
            {
                m_progress.status = Progress::Status::KO;
                m_progress_callback(m_progress);
            }
            return false;
        }

        // Final progress update
        if (m_progress_callback)
        {
            m_progress.status = Progress::Status::OK;
            m_progress_callback(m_progress);
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool extractEntry(std::string const& p_entry_name,
                      std::string const& p_destination_folder,
                      Unzipper::OverwriteMode p_overwrite)
    {
        ZipEntry entry;
        m_error_code.clear();

        // Update progress
        if (m_progress_callback)
        {
            m_progress.reset();
            m_progress_callback(m_progress);
        }

        // Check if the entry exists and get its information
        if (!(locateEntry(p_entry_name) && currentEntryInfo(entry)))
        {
            if (m_progress_callback)
            {
                m_progress.current_file = p_entry_name;
                m_progress.status = Progress::Status::KO;
                m_progress_callback(m_progress);
            }

            return false;
        }

        // Get the canonical output file name
        std::string destination;
        if (!p_destination_folder.empty())
        {
            destination = Path::folderNameWithSeparator(p_destination_folder);
        }
        std::string canon_output_file =
            Path::normalize(destination + p_entry_name);

        // Update progress
        if (m_progress_callback)
        {
            m_progress.total_bytes = entry.uncompressed_size;
            m_progress.current_file = canon_output_file;
            m_progress.status = Progress::Status::InProgress;
            m_progress_callback(m_progress);
        }

        // Extract the current entry to the destination file.
        bool result = extractToFile(entry,
                                    p_destination_folder,
                                    canon_output_file,
                                    p_overwrite) == UNZ_OK;

        // Update progress
        if (m_progress_callback)
        {
            m_progress.files_extracted = result ? 1 : 0;
            m_progress.status =
                result ? Progress::Status::OK : Progress::Status::KO;
            m_progress_callback(m_progress);
        }

        return result;
    }

    // -------------------------------------------------------------------------
    bool extractEntryToStream(std::string const& p_entry_name,
                              std::ostream& p_output_stream)
    {
        ZipEntry entry;
        m_error_code.clear();

        // Update progress
        if (m_progress_callback)
        {
            m_progress.reset();
            m_progress_callback(m_progress);
        }

        // Check if the entry exists and get its information
        if (!(locateEntry(p_entry_name) && currentEntryInfo(entry)))
        {
            if (m_progress_callback)
            {
                m_progress.current_file = p_entry_name;
                m_progress.status = Progress::Status::KO;
                m_progress_callback(m_progress);
            }

            return false;
        }

        // Update progress
        if (m_progress_callback)
        {
            m_progress.total_bytes = entry.uncompressed_size;
            m_progress.current_file = p_entry_name;
            m_progress.status = Progress::Status::InProgress;
            m_progress_callback(m_progress);
        }

        // Extract the current entry to the destination file.
        bool result = extractToStream(p_output_stream, entry) == UNZ_OK;

        // Update progress
        if (m_progress_callback)
        {
            m_progress.files_extracted = result ? 1 : 0;
            m_progress.status =
                result ? Progress::Status::OK : Progress::Status::KO;
            m_progress_callback(m_progress);
        }

        return result;
    }

    // -------------------------------------------------------------------------
    bool extractEntryToMemory(std::string const& p_entry_name,
                              std::vector<unsigned char>& p_output_vector)
    {
        ZipEntry entry;
        m_error_code.clear();

        // Update progress
        if (m_progress_callback)
        {
            m_progress.reset();
            m_progress_callback(m_progress);
        }

        // Check if the entry exists and get its information
        if (!(locateEntry(p_entry_name) && currentEntryInfo(entry)))
        {
            if (m_progress_callback)
            {
                m_progress.current_file = p_entry_name;
                m_progress.status = Progress::Status::KO;
                m_progress_callback(m_progress);
            }

            return false;
        }

        // Update progress
        if (m_progress_callback)
        {
            m_progress.total_bytes = entry.uncompressed_size;
            m_progress.current_file = p_entry_name;
            m_progress.status = Progress::Status::InProgress;
            m_progress_callback(m_progress);
        }

        // Extract the current entry to the destination file.
        bool result = extractToMemory(p_output_vector, entry) == UNZ_OK;

        // Update progress
        if (m_progress_callback)
        {
            m_progress.files_extracted = result ? 1 : 0;
            m_progress.status =
                result ? Progress::Status::OK : Progress::Status::KO;
            m_progress_callback(m_progress);
        }

        return result;
    }
};

// -----------------------------------------------------------------------------
Unzipper::Unzipper() : m_open(false), m_error_code(), m_impl(nullptr) {}

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::istream& p_zipped_buffer, std::string const& p_password)
    : m_impl(std::make_unique<Impl>(p_password, m_error_code))
{
    if (!m_impl->initWithStream(p_zipped_buffer))
    {
        throw std::runtime_error(m_impl->m_error_code.message());
    }
    m_open = true;
}

// -----------------------------------------------------------------------------
Unzipper::Unzipper(const std::vector<unsigned char>& p_zipped_buffer,
                   std::string const& p_password)
    : m_impl(std::make_unique<Impl>(p_password, m_error_code))
{
    if (!m_impl->initWithVector(p_zipped_buffer))
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        throw exception;
    }
    else
    {
        m_open = true;
    }
}

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::string const& p_zipname, std::string const& p_password)
    : m_impl(std::make_unique<Impl>(p_password, m_error_code))
{
    if (!m_impl->initFile(p_zipname))
    {
        throw std::runtime_error(m_impl->m_error_code
                                     ? m_impl->m_error_code.message()
                                     : OS_STRERROR(errno));
    }
    m_open = true;
}

// -----------------------------------------------------------------------------
Unzipper::~Unzipper()
{
    close();
}

// -----------------------------------------------------------------------------
bool Unzipper::open(std::istream& p_zipped_buffer,
                    std::string const& p_password)
{
    if (m_impl != nullptr)
    {
        m_impl->close();
    }
    else
    {
        m_impl = std::make_unique<Impl>(p_password, m_error_code);
    }
    m_open = m_impl->initWithStream(p_zipped_buffer);
    return m_open;
}

// -----------------------------------------------------------------------------
bool Unzipper::open(const std::vector<unsigned char>& p_zipped_buffer,
                    std::string const& p_password)
{
    if (m_impl != nullptr)
    {
        m_impl->close();
    }
    else
    {
        m_impl = std::make_unique<Impl>(p_password, m_error_code);
    }
    m_open = m_impl->initWithVector(p_zipped_buffer);
    return m_open;
}

// -----------------------------------------------------------------------------
bool Unzipper::open(std::string const& p_zipname, std::string const& p_password)
{
    if (m_impl != nullptr)
    {
        m_impl->close();
    }
    else
    {
        m_impl = std::make_unique<Impl>(p_password, m_error_code);
    }
    m_open = m_impl->initFile(p_zipname);
    return m_open;
}

// -----------------------------------------------------------------------------
std::vector<ZipEntry> Unzipper::entries()
{
    if (!checkValid())
        return {};

    return m_impl->entries();
}

// -----------------------------------------------------------------------------
std::vector<ZipEntry> Unzipper::entries(std::string const& p_glob_pattern)
{
    if (!checkValid())
        return {};

    std::vector<ZipEntry> all_entries = m_impl->entries();
    std::vector<ZipEntry> filtered_entries;
    filtered_entries.reserve(all_entries.size());

    for (const auto& entry : all_entries)
    {
        if (p_glob_pattern.empty() ||
            std::regex_match(entry.name, globToRegex(p_glob_pattern)))
        {
            filtered_entries.push_back(entry);
        }
    }

    return filtered_entries;
}

// -----------------------------------------------------------------------------
bool Unzipper::extract(std::string const& p_entry_name,
                       std::string const& p_entry_destination,
                       Unzipper::OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractEntry(p_entry_name, p_entry_destination, p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extract(std::string const& p_entry_name,
                       Unzipper::OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractEntry(p_entry_name, std::string(), p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extract(std::string const& p_entry_name,
                       std::ostream& p_output_stream)
{
    if (!checkValid())
        return false;

    return m_impl->extractEntryToStream(p_entry_name, p_output_stream);
}

// -----------------------------------------------------------------------------
bool Unzipper::extract(std::string const& p_entry_name,
                       std::vector<unsigned char>& p_output_buffer)
{
    if (!checkValid())
        return false;

    return m_impl->extractEntryToMemory(p_entry_name, p_output_buffer);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(
    std::string const& p_folder_destination,
    const std::map<std::string, std::string>& p_alternative_names,
    Unzipper::OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(std::string(),
                              Path::normalize(p_folder_destination),
                              p_alternative_names,
                              p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(std::string const& p_destination,
                          Unzipper::OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(std::string(),
                              Path::normalize(p_destination),
                              std::map<std::string, std::string>(),
                              p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(Unzipper::OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(std::string(),
                              std::string(),
                              std::map<std::string, std::string>(),
                              p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractGlob(
    std::string const& p_glob,
    std::string const& p_folder_destination,
    const std::map<std::string, std::string>& p_alternative_names,
    OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(p_glob,
                              Path::normalize(p_folder_destination),
                              p_alternative_names,
                              p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractGlob(std::string const& p_glob,
                           std::string const& p_folder_destination,
                           OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(p_glob,
                              Path::normalize(p_folder_destination),
                              std::map<std::string, std::string>(),
                              p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractGlob(std::string const& p_glob, OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(p_glob,
                              std::string(),
                              std::map<std::string, std::string>(),
                              p_overwrite);
}

// -----------------------------------------------------------------------------
void Unzipper::close()
{
    if (m_open && m_impl)
    {
        m_impl->close();
    }
    m_open = false;
    m_error_code.clear();
}

// -----------------------------------------------------------------------------
bool Unzipper::checkValid()
{
    if (!m_impl)
    {
        m_error_code = make_error_code(UnzipperError::INTERNAL_ERROR,
                                       "Unzipper is not initialized");
        return false;
    }

    if (!m_open)
    {
        m_error_code = make_error_code(UnzipperError::OPENING_ERROR,
                                       "Zip archive is not opened");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
size_t Unzipper::sizeOnDisk()
{
    size_t total_uncompressed = 0;
    auto entries = m_impl->entries();
    for (const auto& entry : entries)
    {
        total_uncompressed += entry.uncompressed_size;
    }
    return total_uncompressed;
}

// -----------------------------------------------------------------------------
bool Unzipper::setProgressCallback(ProgressCallback callback)
{
    if (m_impl)
    {
        m_impl->m_progress_callback = std::move(callback);
        return true;
    }
    return false;
}
} // namespace zipper
