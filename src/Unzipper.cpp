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
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>

#ifndef ZIPPER_WRITE_BUFFER_SIZE
#    define ZIPPER_WRITE_BUFFER_SIZE (32768u)
#endif

namespace zipper {

enum class unzipper_error
{
    NO_ERROR_UNZIPPER = 0,
    //! Error when accessing to a info entry
    NO_ENTRY,
    //! Error inside libraries
    INTERNAL_ERROR,
    //! Zip slip vulnerability
    //! https://snyk.io/research/zip-slip-vulnerability
    SECURITY_ERROR,
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

    virtual std::string message(int p_error) const override
    {
        if (!custom_message.empty())
        {
            return custom_message;
        }

        switch (static_cast<unzipper_error>(p_error))
        {
            case unzipper_error::NO_ERROR_UNZIPPER:
                return "There was no error";
            case unzipper_error::NO_ENTRY:
                return "Error, couldn't get the current entry info";
            case unzipper_error::INTERNAL_ERROR:
                return "Internal error";
            case unzipper_error::SECURITY_ERROR:
                return "ZipSlip security";
            default:
                return "Unkown Error";
        }
    }

    std::string custom_message;
};

// -----------------------------------------------------------------------------
static UnzipperErrorCategory theUnzipperErrorCategory;

// -----------------------------------------------------------------------------
static std::error_code make_error_code(unzipper_error p_error)
{
    return {static_cast<int>(p_error), theUnzipperErrorCategory};
}

// -----------------------------------------------------------------------------
static std::error_code make_error_code(unzipper_error p_error,
                                       std::string const& p_message)
{
    // std::cerr << message << std::endl;
    theUnzipperErrorCategory.custom_message = p_message;
    return {static_cast<int>(p_error), theUnzipperErrorCategory};
}

// *************************************************************************
//! \brief PIMPL implementation
// *************************************************************************
struct Unzipper::Impl
{
    Unzipper& m_outer;
    zipFile m_zip_file;
    ourmemory_t m_zip_memory;
    zlib_filefunc_def m_file_func;
    std::error_code& m_error_code;
    std::vector<char> m_char_buffer;
    std::vector<unsigned char> m_uchar_buffer;

private:

    // -------------------------------------------------------------------------
    bool initMemory(zlib_filefunc_def& p_file_func)
    {
        m_zip_file = unzOpen2("__notused__", &p_file_func);
        if (m_zip_file == nullptr)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                           strerror(errno));
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool locateEntry(std::string const& p_entry_name)
    {
        bool res = (UNZ_OK ==
                    unzLocateFile(m_zip_file, p_entry_name.c_str(), nullptr));
        if (!res)
        {
            std::stringstream str;
            str << "Invalid info entry '"
                << Path::toNativeSeparators(p_entry_name) << "'";
            m_error_code = make_error_code(unzipper_error::NO_ENTRY, str.str());
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool failIfInvalidEntry(ZipEntry const& p_entry_info)
    {
        if (!p_entry_info.valid())
        {
            std::stringstream str;
            str << "Invalid info entry '"
                << Path::toNativeSeparators(p_entry_info.name) << "'";
            m_error_code = make_error_code(unzipper_error::NO_ENTRY, str.str());
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
            m_zip_file, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Invalid info entry '"
                << Path::toNativeSeparators(p_entry_info.name) << "'";
            m_error_code = make_error_code(unzipper_error::NO_ENTRY, str.str());
            return false;
        }

        // Dynamic allocation of the exact size needed
        std::vector<char> filename_inzip(file_info.size_filename + 1, 0);

        // Second pass to get the file name
        err = unzGetCurrentFileInfo64(m_zip_file,
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
            str << "Invalid info entry '"
                << Path::toNativeSeparators(p_entry_info.name) << "'";
            m_error_code = make_error_code(unzipper_error::NO_ENTRY, str.str());
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
    void getEntries(std::vector<ZipEntry>& entries)
    {
        // First pass to count the number of entries
        uLong numEntries = 0;
        unz_global_info64 gi;
        int err = unzGetGlobalInfo64(m_zip_file, &gi);
        if (err == UNZ_OK)
        {
            numEntries = gi.number_entry;

            // Pre-allocation of the vector to avoid reallocations
            entries.reserve(numEntries);

            // Second pass to get the entries
            err = unzGoToFirstFile(m_zip_file);
            if (UNZ_OK == err)
            {
                // Traverse the zip file sequentially in a single pass
                do
                {
                    // Get the current entry info
                    ZipEntry entryinfo;
                    if (currentEntryInfo(entryinfo) && entryinfo.valid())
                    {
                        entries.push_back(entryinfo);
                        err = unzGoToNextFile(m_zip_file);
                    }
                    else
                    {
                        // Set the error code if the entry info is invalid
                        err = UNZ_ERRNO;
                        m_error_code = make_error_code(
                            unzipper_error::INTERNAL_ERROR, strerror(errno));
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
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                           "Failed to get global zip info");
        }
    }

public:

    // -------------------------------------------------------------------------
    bool extractCurrentEntryToFile(ZipEntry& entryinfo,
                                   std::string const& fileName,
                                   bool const replace)
    {
        int err = UNZ_OK;

        if (!failIfInvalidEntry(entryinfo))
            return false;

        // if (!entryinfo.uncompressedSize) was not a good method to distinguish
        // dummy file from folder. See
        // https://github.com/Lecrapouille/zipper/issues/5
        if (Path::hasTrailingSlash(entryinfo.name))
        {
            if (!Path::createDir(fileName))
            {
                std::stringstream str;
                str << "Error cannot create the folder '"
                    << Path::toNativeSeparators(fileName) << "'";
                m_error_code =
                    make_error_code(unzipper_error::INTERNAL_ERROR, str.str());
                err = UNZ_ERRNO;
            }
        }
        else
        {
            err = extractToFile(fileName, entryinfo, replace);
            if (UNZ_OK == err)
            {
                err = unzCloseCurrentFile(m_zip_file);
                if (UNZ_OK != err)
                {
                    std::stringstream str;
                    str << "Error opening internal file '" << entryinfo.name
                        << "' in zip";
                    m_error_code = make_error_code(
                        unzipper_error::INTERNAL_ERROR, str.str());
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

        if (!failIfInvalidEntry(p_entry_info))
            return false;

        err = extractToStream(p_stream, p_entry_info);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zip_file);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error opening internal file '" << p_entry_info.name
                    << "' in zip";

                m_error_code =
                    make_error_code(unzipper_error::INTERNAL_ERROR, str.str());
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

        if (!failIfInvalidEntry(p_entry_info))
            return false;

        err = extractToMemory(p_output_vector, p_entry_info);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zip_file);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error opening internal file '" << p_entry_info.name
                    << "' in zip";

                m_error_code =
                    make_error_code(unzipper_error::INTERNAL_ERROR, str.str());
            }
        }

        return UNZ_OK == err;
    }

    // -------------------------------------------------------------------------
    void changeFileDate(std::string const& p_filename,
                        uLong p_dos_date,
                        tm_zip const& p_tmu_date)
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
                      bool const p_replace)
    {
        // If zip entry is a directory then create it on disk
        std::string folder = Path::dirName(p_filename);
        if (!folder.empty())
        {
            std::string canon = Path::canonicalPath(folder);
            if (!canon.empty() && canon.find("..") != std::string::npos)
            {
                // Prevent Zip Slip attack (See ticket #33)
                std::stringstream str;
                str << "Security error: entry '"
                    << Path::toNativeSeparators(p_filename)
                    << "' would be outside your target directory";

                m_error_code =
                    make_error_code(unzipper_error::SECURITY_ERROR, str.str());
                return UNZ_ERRNO;
            }
            if (!Path::createDir(folder))
            {
                std::stringstream str;
                str << "Error: cannot create the folder '"
                    << Path::toNativeSeparators(folder) << "'";

                m_error_code =
                    make_error_code(unzipper_error::INTERNAL_ERROR, str.str());
                return UNZ_ERRNO;
            }
        }

        // Avoid replacing the file. Prevent Zip Slip attack (See ticket #33)
        if (!p_replace && Path::exist(p_filename))
        {
            std::stringstream str;
            str << "Security Error: '" << Path::toNativeSeparators(p_filename)
                << "' already exists and would have been replaced!";

            m_error_code =
                make_error_code(unzipper_error::SECURITY_ERROR, str.str());
            return UNZ_ERRNO;
        }

        // Check if the filename is valid
        std::string canon = Path::canonicalPath(p_filename);
        if (!canon.empty() && canon.find("..") != std::string::npos)
        {
            std::stringstream str;
            str << "Security error: entry '"
                << Path::toNativeSeparators(p_filename)
                << "' would be outside your target directory";

            m_error_code =
                make_error_code(unzipper_error::SECURITY_ERROR, str.str());
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
            memcpy(&timeaux, &p_entry_info.unixdate, sizeof(timeaux));

            changeFileDate(p_filename.c_str(), p_entry_info.dosdate, timeaux);
            return err;
        }
        else
        {
            // Possible error is a directory already exists. The errno message
            // is not very explicit.
            std::stringstream str;
            str << "Failed creating '" << Path::toNativeSeparators(p_filename)
                << "' file because " << strerror(errno);

            m_error_code =
                make_error_code(unzipper_error::INTERNAL_ERROR, str.str());
            output_file.close();
            return UNZ_ERRNO;
        }
    }

    // -------------------------------------------------------------------------
    int extractToStream(std::ostream& stream, ZipEntry& /*p_entry_info*/)
    {
        int err;
        int bytes = 0;

        err = unzOpenCurrentFilePassword(
            m_zip_file,
            m_outer.m_password.empty() ? NULL : m_outer.m_password.c_str());
        if (UNZ_OK == err)
        {
            do
            {
                bytes = unzReadCurrentFile(
                    m_zip_file,
                    m_char_buffer.data(),
                    static_cast<unsigned int>(m_char_buffer.size()));
                if (bytes <= 0)
                {
                    if (bytes == 0)
                        break; // If password was not valid we reach this case.

                    m_error_code = make_error_code(
                        unzipper_error::INTERNAL_ERROR, strerror(errno));
                    return UNZ_ERRNO;
                }

                stream.write(m_char_buffer.data(), std::streamsize(bytes));
                if (!stream.good())
                {
                    m_error_code = make_error_code(
                        unzipper_error::INTERNAL_ERROR, strerror(errno));
                    return UNZ_ERRNO;
                }
            } while (bytes > 0);

            stream.flush();
        }
        else
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR);
        }
        return err;
    }

    // -------------------------------------------------------------------------
    int extractToMemory(std::vector<unsigned char>& outvec, ZipEntry& p_info)
    {
        int err;
        int bytes = 0;

        err = unzOpenCurrentFilePassword(
            m_zip_file,
            m_outer.m_password.empty() ? NULL : m_outer.m_password.c_str());
        if (UNZ_OK == err)
        {
            // Pre-allocation to avoid costly reallocations
            outvec.clear();
            outvec.reserve(static_cast<size_t>(p_info.uncompressedSize));

            do
            {
                bytes = unzReadCurrentFile(
                    m_zip_file,
                    m_uchar_buffer.data(),
                    static_cast<unsigned int>(m_uchar_buffer.size()));
                if (bytes <= 0)
                {
                    if (bytes == 0)
                        break;

                    m_error_code = make_error_code(
                        unzipper_error::INTERNAL_ERROR, strerror(errno));
                    return UNZ_ERRNO;
                }

                // Use of insert with iterators for better performance
                outvec.insert(outvec.end(),
                              m_uchar_buffer.data(),
                              m_uchar_buffer.data() + bytes);
            } while (bytes > 0);
        }
        else
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR);
        }

        return err;
    }

public:

    // -------------------------------------------------------------------------
    Impl(Unzipper& p_outer, std::error_code& p_error_code)
        : m_outer(p_outer),
          m_zip_memory(),
          m_file_func(),
          m_error_code(p_error_code),
          m_char_buffer(ZIPPER_WRITE_BUFFER_SIZE),
          m_uchar_buffer(ZIPPER_WRITE_BUFFER_SIZE)
    {
        m_zip_memory.base = nullptr;
        m_zip_file = nullptr;
    }

    // -------------------------------------------------------------------------
    ~Impl()
    {
        close();
    }

    // -------------------------------------------------------------------------
    void close()
    {
        if (m_zip_file != nullptr)
        {
            unzClose(m_zip_file);
            m_zip_file = nullptr;
        }

        if (m_zip_memory.base != nullptr)
        {
            free(m_zip_memory.base);
            m_zip_memory.base = nullptr;
        }

        // Free the memory of the buffers
        std::vector<char>().swap(m_char_buffer);
        std::vector<unsigned char>().swap(m_uchar_buffer);
    }

    // -------------------------------------------------------------------------
    bool initFile(std::string const& filename)
    {
        if (!Path::exist(filename))
        {
            m_error_code =
                make_error_code(unzipper_error::NO_ENTRY, "Does not exist");
            return false;
        }
        if (!Path::isFile(filename))
        {
            m_error_code =
                make_error_code(unzipper_error::NO_ENTRY, "Not a zip file");
            return false;
        }

#if defined(_WIN32)
        zlib_filefunc64_def ffunc;
        fill_win32_filefunc64A(&ffunc);
        m_zip_file = unzOpen2_64(filename.c_str(), &ffunc);
#else
        m_zip_file = unzOpen64(filename.c_str());
#endif

        if (m_zip_file != nullptr)
            return true;

        if (filename.substr(filename.find_last_of(".") + 1u) != "zip")
        {
            m_error_code =
                make_error_code(unzipper_error::NO_ENTRY, "Not a zip file");
            return false;
        }

        // https://github.com/sebastiandev/zipper/issues/118
        // Avoid throwing error if the zip is empty. Having a dummy zip file is
        // probably weird but we may want get a dummy array when calling
        // Unzipper::entries(); or add files with Zipper::add().
        return true;
    }

    // -------------------------------------------------------------------------
    bool initWithStream(std::istream& stream)
    {
        stream.seekg(0, std::ios::end);
        std::streampos s = stream.tellg();
        stream.seekg(0);

        if (s < 0)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                           strerror(errno));
            return false;
        }
        else
        {
            size_t size = static_cast<size_t>(s);
            m_zip_memory.base = static_cast<char*>(malloc(size * sizeof(char)));
            if (m_zip_memory.base == nullptr)
            {
                m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                               "Failed to allocate memory");
                return false;
            }
            m_zip_memory.size = static_cast<uint32_t>(size);
            stream.read(m_zip_memory.base, std::streamsize(size));
            if (!stream.good())
            {
                m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                               strerror(errno));
                return false;
            }
        }

        fill_memory_filefunc(&m_file_func, &m_zip_memory);
        return initMemory(m_file_func);
    }

    // -------------------------------------------------------------------------
    bool initWithVector(std::vector<unsigned char>& buffer)
    {
        if (!buffer.empty())
        {
            m_zip_memory.base =
                reinterpret_cast<char*>(malloc(buffer.size() * sizeof(char)));
            memcpy(m_zip_memory.base,
                   reinterpret_cast<char*>(buffer.data()),
                   buffer.size());
            m_zip_memory.size = static_cast<uint32_t>(buffer.size());
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
    bool extractAll(std::string const& destination,
                    const std::map<std::string, std::string>& alternativeNames,
                    bool const replace)
    {
        bool res = true;

        // Traverse the zip file sequentially in a single pass
        int err = unzGoToFirstFile(m_zip_file);
        if (err != UNZ_OK)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                           "Failed to go to first file");
            return false;
        }

        // Preparation of the destination prefix once
        std::string destPrefix =
            destination.empty() ? ""
                                : Path::folderNameWithSeparator(destination);

        do
        {
            ZipEntry entry;
            if (!currentEntryInfo(entry) || !entry.valid())
            {
                res = false;
                continue;
            }

            std::string alternativeName = destPrefix;

            auto const& alt = alternativeNames.find(entry.name);
            if (alt != alternativeNames.end())
                alternativeName += alt->second;
            else
                alternativeName += entry.name;

            if (!extractCurrentEntryToFile(entry, alternativeName, replace))
            {
                res = false;
            }

            err = unzGoToNextFile(m_zip_file);
        } while (err == UNZ_OK);

        if (err != UNZ_END_OF_LIST_OF_FILE && err != UNZ_OK)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                           "Error navigating zip file");
            return false;
        }

        return res;
    }

    // -------------------------------------------------------------------------
    bool extractEntry(std::string const& p_name,
                      std::string const& p_destination,
                      bool const p_replace)
    {
        ZipEntry entry;
        std::string outputFile =
            p_destination.empty()
                ? p_name
                : Path::folderNameWithSeparator(p_destination) + p_name;
        std::string canonOutputFile = Path::canonicalPath(outputFile);

        return locateEntry(p_name) && currentEntryInfo(entry) &&
               extractCurrentEntryToFile(entry, canonOutputFile, p_replace);
    }

    // -------------------------------------------------------------------------
    bool extractEntryToStream(std::string const& p_name, std::ostream& p_stream)
    {
        ZipEntry entry;

        return locateEntry(p_name) && currentEntryInfo(entry) &&
               extractCurrentEntryToStream(entry, p_stream);
    }

    // -------------------------------------------------------------------------
    bool extractEntryToMemory(std::string const& p_name,
                              std::vector<unsigned char>& p_vec)
    {
        ZipEntry entry;

        return locateEntry(p_name) && currentEntryInfo(entry) &&
               extractCurrentEntryToMemory(entry, p_vec);
    }
};

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::istream& p_zipped_buffer, std::string const& p_password)
    : m_ibuffer(p_zipped_buffer),
      m_vecbuffer(*(new std::vector<unsigned char>())),
      m_password(p_password),
      m_usingMemoryVector(false),
      m_usingStream(true),
      m_impl(new Impl(*this, m_error_code))
{
    if (!m_impl->initWithStream(m_ibuffer))
    {
        release();
        throw std::runtime_error(m_impl->m_error_code.message());
    }
    m_open = true;
}

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::vector<unsigned char>& p_zipped_buffer,
                   std::string const& p_password)
    : m_ibuffer(*(new std::stringstream())),
      m_vecbuffer(p_zipped_buffer),
      m_password(p_password),
      m_usingMemoryVector(true),
      m_usingStream(false),
      m_impl(new Impl(*this, m_error_code))
{
    // if (zippedBuffer.empty())
    //{
    //     release();
    //     throw std::runtime_error("std::vector shall not be empty");
    // }
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    else
    {
        m_open = true;
    }
}

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::string const& p_zipname, std::string const& p_password)
    : m_ibuffer(*(new std::stringstream())),
      m_vecbuffer(*(new std::vector<unsigned char>())),
      m_zipname(p_zipname),
      m_password(p_password),
      m_usingMemoryVector(false),
      m_usingStream(false),
      m_impl(new Impl(*this, m_error_code))
{
    if (!m_impl->initFile(p_zipname))
    {
        if (m_impl->m_error_code)
        {
            std::runtime_error exception(m_impl->m_error_code.message());
            release();
            throw exception;
        }
    }
    else
    {
        m_open = true;
    }
}

// -----------------------------------------------------------------------------
Unzipper::~Unzipper()
{
    close();
    release();
}

// -----------------------------------------------------------------------------
std::vector<ZipEntry> Unzipper::entries()
{
    if ((m_open == false) || (m_impl == nullptr))
        return {};
    return m_impl->entries();
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntry(std::string const& p_entry_name,
                            std::string const& p_entry_destination,
                            bool const p_replace)
{
    if ((m_open == false) || (m_impl == nullptr))
        return false;
    return m_impl->extractEntry(p_entry_name, p_entry_destination, p_replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntry(std::string const& p_entry_name,
                            bool const p_replace)
{
    if ((m_open == false) || (m_impl == nullptr))
        return false;
    return m_impl->extractEntry(p_entry_name, std::string(), p_replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntryToStream(std::string const& p_entry_name,
                                    std::ostream& p_output_stream)
{
    if ((m_open == false) || (m_impl == nullptr))
        return false;
    return m_impl->extractEntryToStream(p_entry_name, p_output_stream);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntryToMemory(std::string const& p_entry_name,
                                    std::vector<unsigned char>& p_output_buffer)
{
    if ((m_open == false) || (m_impl == nullptr))
        return false;
    return m_impl->extractEntryToMemory(p_entry_name, p_output_buffer);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(
    std::string const& p_folder_destination,
    const std::map<std::string, std::string>& p_alternative_names,
    bool const p_replace)
{
    if ((m_open == false) || (m_impl == nullptr))
        return false;
    return m_impl->extractAll(Path::canonicalPath(p_folder_destination),
                              p_alternative_names,
                              p_replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(bool p_replace)
{
    if ((m_open == false) || (m_impl == nullptr))
        return false;
    return m_impl->extractAll(
        std::string(), std::map<std::string, std::string>(), p_replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(std::string const& p_destination,
                          bool const p_replace)
{
    if ((m_open == false) || (m_impl == nullptr))
        return false;
    return m_impl->extractAll(Path::canonicalPath(p_destination),
                              std::map<std::string, std::string>(),
                              p_replace);
}

// -----------------------------------------------------------------------------
void Unzipper::release()
{
    if (!m_usingMemoryVector)
    {
        delete &m_vecbuffer;
    }
    if (!m_usingStream)
    {
        delete &m_ibuffer;
    }
    if (m_impl != nullptr)
    {
        delete m_impl;
    }
}

// -----------------------------------------------------------------------------
void Unzipper::close()
{
    if (m_open && (m_impl != nullptr))
    {
        m_impl->close();
    }
    m_open = false;
    m_error_code.clear();
}

// -----------------------------------------------------------------------------
std::error_code const& Unzipper::error() const
{
    return m_error_code;
}

} // namespace zipper
