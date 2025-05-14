//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#include "Zipper/Unzipper.hpp"
#include "utils/OS.hpp"
#include "utils/Path.hpp"

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

namespace zipper {

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
    return {static_cast<int>(p_error), theUnzipperErrorCategory};
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
        if (p_entry_name.empty() ||
            (unzLocateFile(m_zip_handler, p_entry_name.c_str(), nullptr) !=
             UNZ_OK))
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
    bool currentEntryInfo(ZipEntry& p_entry_info)
    {
        unz_file_info64 file_info;

        // First pass to get the file name size
        int err = unzGetCurrentFileInfo64(
            m_zip_handler, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Invalid zip entry info '"
                << Path::toNativeSeparators(p_entry_info.name) << "'";
            m_error_code = make_error_code(UnzipperError::BAD_ENTRY, str.str());
            return false;
        }

        // Dynamic allocation of the exact size needed
        std::vector<char> filename_inzip(file_info.size_filename + 1, 0);

        // Second pass to get the file name
        err = unzGetCurrentFileInfo64(m_zip_handler,
                                      &file_info,
                                      filename_inzip.data(),
                                      file_info.size_filename + 1,
                                      nullptr,
                                      0,
                                      nullptr,
                                      0);
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Invalid zip entry info '"
                << Path::toNativeSeparators(p_entry_info.name) << "'";
            m_error_code = make_error_code(UnzipperError::BAD_ENTRY, str.str());
            return false;
        }

        p_entry_info = ZipEntry(std::string(filename_inzip.data()),
                                file_info.compressed_size,
                                file_info.uncompressed_size,
                                file_info.tmu_date.tm_year,
                                file_info.tmu_date.tm_mon,
                                file_info.tmu_date.tm_mday,
                                file_info.tmu_date.tm_hour,
                                file_info.tmu_date.tm_min,
                                file_info.tmu_date.tm_sec,
                                file_info.dos_date);
        return true;
    }

    // -------------------------------------------------------------------------
    bool isEntryValid(ZipEntry const& p_entry_info)
    {
        return !p_entry_info.name.empty();
    }

    // -------------------------------------------------------------------------
    void getEntries(std::vector<ZipEntry>& p_entries)
    {
        // First pass to count the number of entries
        uint64_t num_entries = 0;
        unz_global_info64 gi;
        int err = unzGetGlobalInfo64(m_zip_handler, &gi);
        if (err == UNZ_OK)
        {
            num_entries = gi.number_entry;

            // Pre-allocation of the vector to avoid reallocations
            p_entries.reserve(num_entries);

            // Second pass to get the entries
            err = unzGoToFirstFile(m_zip_handler);
            if (UNZ_OK == err)
            {
                // Traverse the zip file sequentially in a single pass
                do
                {
                    // Get the current entry info
                    ZipEntry entry_info;
                    if (currentEntryInfo(entry_info) &&
                        isEntryValid(entry_info))
                    {
                        p_entries.push_back(entry_info);
                        err = unzGoToNextFile(m_zip_handler);
                    }
                    else
                    {
                        // Set the error code if the entry info is invalid
                        err = UNZ_ERRNO;
                        m_error_code = make_error_code(
                            UnzipperError::INTERNAL_ERROR, OS_STRERROR(errno));
                    }
                } while (UNZ_OK == err);

                // If the end of the list of files is not reached and there is
                // an error, return
                if (UNZ_END_OF_LIST_OF_FILE != err && UNZ_OK != err)
                {
                    return;
                }
            }
        }
        else
        {
            m_error_code = make_error_code(UnzipperError::INTERNAL_ERROR,
                                           "Invalid zip entry info");
        }
    }

public:

    // -------------------------------------------------------------------------
    bool extractCurrentEntryToFile(ZipEntry& p_entry_info,
                                   std::string const& p_file_name,
                                   Unzipper::OverwriteMode p_overwrite)
    {
        int err = UNZ_OK;

        // if (!entryinfo.uncompressed_size) was not a good method to
        // distinguish dummy file from folder. See
        // https://github.com/Lecrapouille/zipper/issues/5
        if (Path::hasTrailingSlash(p_entry_info.name))
        {
            // Folder name may have an extension file, so we do not add checks
            // if folder name ends with folder slash.
            if (!Path::createDir(p_file_name))
            {
                std::stringstream str;
                str << "Failed creating folder '"
                    << Path::toNativeSeparators(p_file_name) << "'";
                m_error_code =
                    make_error_code(UnzipperError::EXTRACT_ERROR, str.str());
                err = UNZ_ERRNO;
            }
        }
        else
        {
            // Update progress before starting extraction
            if (m_progress_callback)
            {
                m_progress.current_file = p_entry_info.name;
                m_progress.status = Progress::Status::InProgress;
                m_progress_callback(m_progress);
            }

            err = extractToFile(p_file_name, p_entry_info, p_overwrite);
            if (UNZ_OK == err)
            {
                err = unzCloseCurrentFile(m_zip_handler);
                if (UNZ_OK != err)
                {
                    std::stringstream str;
                    str << "Failed closing file '" << p_entry_info.name << "'";
                    m_error_code = make_error_code(
                        UnzipperError::INTERNAL_ERROR, str.str());
                    return false;
                }
            }
        }

        return UNZ_OK == err;
    }

    // -------------------------------------------------------------------------
    bool extractCurrentEntryToStream(ZipEntry& p_entry_info,
                                     std::ostream& p_stream)
    {
        int err = UNZ_OK;

        // Update progress before starting extraction
        if (m_progress_callback)
        {
            m_progress.current_file = p_entry_info.name;
            m_progress.status = Progress::Status::InProgress;
            m_progress_callback(m_progress);
        }

        err = extractToStream(p_stream, p_entry_info);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zip_handler);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Failed closing file '" << p_entry_info.name << "'";
                m_error_code =
                    make_error_code(UnzipperError::INTERNAL_ERROR, str.str());
            }
        }

        return UNZ_OK == err;
    }

    // -------------------------------------------------------------------------
    bool
    extractCurrentEntryToMemory(ZipEntry& p_entry_info,
                                std::vector<unsigned char>& p_output_vector)
    {
        int err = UNZ_OK;

        // Update progress before starting extraction
        if (m_progress_callback)
        {
            m_progress.current_file = p_entry_info.name;
            m_progress.status = Progress::Status::InProgress;
            m_progress_callback(m_progress);
        }

        err = extractToMemory(p_output_vector, p_entry_info);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zip_handler);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Failed closing file '" << p_entry_info.name << "'";
                m_error_code =
                    make_error_code(UnzipperError::INTERNAL_ERROR, str.str());
            }
        }

        return UNZ_OK == err;
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
    int extractToFile(std::string const& p_filename,
                      ZipEntry& p_entry_info,
                      Unzipper::OverwriteMode p_overwrite)
    {
        // If zip file path contains a directory then create it on disk
        std::string folder = Path::dirName(p_filename);
        if (!folder.empty())
        {
            std::string canon = Path::normalize(folder);
            if (!canon.empty() && canon.find("..") != std::string::npos)
            {
                // Prevent Zip Slip attack (See ticket #33)
                std::stringstream str;
                str << "Security error: entry '"
                    << Path::toNativeSeparators(p_filename)
                    << "' would be outside your target directory";

                m_error_code =
                    make_error_code(UnzipperError::SECURITY_ERROR, str.str());
                return UNZ_ERRNO;
            }
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

        // Avoid replacing the file. Prevent Zip Slip attack (See ticket #33)
        if ((p_overwrite == Unzipper::OverwriteMode::DoNotOverwrite) &&
            Path::exist(p_filename))
        {
            std::stringstream str;
            str << "Security Error: '" << Path::toNativeSeparators(p_filename)
                << "' already exists and would have been replaced!";

            m_error_code =
                make_error_code(UnzipperError::SECURITY_ERROR, str.str());
            return UNZ_ERRNO;
        }

        // Check if the filename is valid
        std::string canon = Path::normalize(p_filename);
        if (!canon.empty() && canon.find("..") != std::string::npos)
        {
            std::stringstream str;
            str << "Security error: entry '"
                << Path::toNativeSeparators(p_filename)
                << "' would be outside your target directory";

            m_error_code =
                make_error_code(UnzipperError::EXTRACT_ERROR, str.str());
            return UNZ_ERRNO;
        }

        // Create the file on disk so we can unzip to it
        std::ofstream output_file(p_filename.c_str(), std::ofstream::binary);
        if (output_file.good())
        {
            int err = extractToStream(output_file, p_entry_info);

            output_file.close();

            // Set the time of the file that has been unzipped
            tm_zip timeaux;
            memcpy(&timeaux, &p_entry_info.unix_date, sizeof(timeaux));

            changeFileDate(p_filename.c_str(), p_entry_info.dos_date, timeaux);
            return err;
        }
        else
        {
            // Possible error is a directory already exists. The errno message
            // is not very explicit.
            std::stringstream str;
            str << "Failed creating file '"
                << Path::toNativeSeparators(p_filename)
                << "' because: " << OS_STRERROR(errno);

            m_error_code =
                make_error_code(UnzipperError::EXTRACT_ERROR, str.str());
            output_file.close();
            return UNZ_ERRNO;
        }
    }

    // -------------------------------------------------------------------------
    int extractToStream(std::ostream& p_stream, const ZipEntry& p_entry_info)
    {
        int err;
        int bytes = 0;

        // Update progress
        if (m_progress_callback)
        {
            m_progress.current_file = p_entry_info.name;
        }

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
    std::vector<ZipEntry> entries()
    {
        std::vector<ZipEntry> entrylist;
        getEntries(entrylist);
        return entrylist;
    }

    // -------------------------------------------------------------------------
    bool
    extractAll(std::string const& p_destination,
               const std::map<std::string, std::string>& p_alternative_names,
               Unzipper::OverwriteMode p_overwrite)
    {
        m_error_code.clear();
        bool res = true;

        if (m_progress_callback)
        {
            // Get total number of files and bytes
            m_progress.total_bytes = 0;
            std::vector<ZipEntry> entries = this->entries();
            for (const auto& entry : entries)
            {
                m_progress.total_bytes += entry.uncompressed_size;
            }

            // Initialize progress
            m_progress.status = Progress::Status::InProgress;
            m_progress.total_files = entries.size();
            m_progress.files_extracted = 0;
            m_progress.bytes_read = 0;
            m_progress_callback(m_progress);
        }

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

        // Preparation of the destination prefix once
        std::string destPrefix =
            p_destination.empty()
                ? ""
                : Path::folderNameWithSeparator(p_destination);

        do
        {
            ZipEntry entry;
            if ((!currentEntryInfo(entry)) || (!isEntryValid(entry)))
            {
                res = false;
                continue;
            }

            std::string alternative_name = destPrefix;

            auto const& alt = p_alternative_names.find(entry.name);
            if (alt != p_alternative_names.end())
                alternative_name += alt->second;
            else
                alternative_name += entry.name;

            if (!extractCurrentEntryToFile(
                    entry, alternative_name, p_overwrite))
            {
                res = false;
            }

            // Update progress after each file
            else if (m_progress_callback)
            {
                m_progress.files_extracted++;
                m_progress_callback(m_progress);
            }

            err = unzGoToNextFile(m_zip_handler);
        } while (err == UNZ_OK);

        if (err != UNZ_END_OF_LIST_OF_FILE && err != UNZ_OK)
        {
            m_error_code =
                make_error_code(UnzipperError::INTERNAL_ERROR,
                                "Failed navigating inside zip entries");
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
            m_progress.status =
                res ? Progress::Status::OK : Progress::Status::KO;
            m_progress_callback(m_progress);
        }

        return res;
    }

    // -------------------------------------------------------------------------
    bool extractEntry(std::string const& p_name,
                      std::string const& p_destination,
                      Unzipper::OverwriteMode p_overwrite)
    {
        ZipEntry entry;
        std::string outputFile =
            p_destination.empty()
                ? p_name
                : Path::folderNameWithSeparator(p_destination) + p_name;
        std::string canonOutputFile = Path::normalize(outputFile);

        m_error_code.clear();
        return locateEntry(p_name) && currentEntryInfo(entry) &&
               extractCurrentEntryToFile(entry, canonOutputFile, p_overwrite);
    }

    // -------------------------------------------------------------------------
    bool extractEntryToStream(std::string const& p_name, std::ostream& p_stream)
    {
        ZipEntry entry;

        m_error_code.clear();
        return locateEntry(p_name) && currentEntryInfo(entry) &&
               extractCurrentEntryToStream(entry, p_stream);
    }

    // -------------------------------------------------------------------------
    bool extractEntryToMemory(std::string const& p_name,
                              std::vector<unsigned char>& p_vec)
    {
        ZipEntry entry;

        m_error_code.clear();
        return locateEntry(p_name) && currentEntryInfo(entry) &&
               extractCurrentEntryToMemory(entry, p_vec);
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
std::vector<ZipEntry> Unzipper::entries()
{
    if (!checkValid())
        return {};

    return m_impl->entries();
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

    return m_impl->extractAll(Path::normalize(p_folder_destination),
                              p_alternative_names,
                              p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(Unzipper::OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(
        std::string(), std::map<std::string, std::string>(), p_overwrite);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(std::string const& p_destination,
                          Unzipper::OverwriteMode p_overwrite)
{
    if (!checkValid())
        return false;

    return m_impl->extractAll(Path::normalize(p_destination),
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
