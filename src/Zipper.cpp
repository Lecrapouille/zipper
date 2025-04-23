//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#include "Zipper/Zipper.hpp"
#include "utils/OS.hpp"
#include "utils/Path.hpp"
#include "utils/Timestamp.hpp"

#include "external/minizip/ioapi_mem.h"
#include "external/minizip/zip.h"

#include <fstream>
#include <stdexcept>

#ifndef ZIPPER_WRITE_BUFFER_SIZE
#    define ZIPPER_WRITE_BUFFER_SIZE (65536u)
#endif

namespace zipper {

enum class ZipperError
{
    NO_ERROR_ZIPPER = 0,
    OPENING_ERROR,
    INTERNAL_ERROR,
    NO_ENTRY,
    SECURITY_ERROR
};

// *************************************************************************
//! \brief std::error_code instead of throw() or errno.
// *************************************************************************
struct ZipperErrorCategory: std::error_category
{
    virtual const char* name() const noexcept override
    {
        return "zipper";
    }

    virtual std::string message(int ev) const override
    {
        if (!custom_message.empty())
        {
            return custom_message;
        }

        switch (static_cast<ZipperError>(ev))
        {
            case ZipperError::NO_ERROR_ZIPPER:
                return "There was no error";
            case ZipperError::OPENING_ERROR:
                return "Opening error";
            case ZipperError::INTERNAL_ERROR:
                return "Internal error";
            case ZipperError::NO_ENTRY:
                return "Error, couldn't get the current entry info";
            case ZipperError::SECURITY_ERROR:
                return "ZipSlip security";
            default:
                return "Unkown Error";
        }
    }

    std::string custom_message;
};

// -----------------------------------------------------------------------------
static ZipperErrorCategory theZipperErrorCategory;

// -----------------------------------------------------------------------------
static std::error_code make_error_code(ZipperError p_error,
                                       std::string const& p_message)
{
    // std::cerr << message << std::endl;
    theZipperErrorCategory.custom_message = p_message;
    return {static_cast<int>(p_error), theZipperErrorCategory};
}

// -----------------------------------------------------------------------------
// Calculate the CRC32 of a file because to encrypt a file, we need known the
// CRC32 of the file before.
static void getFileCrc(std::istream& p_input_stream,
                       std::vector<char>& p_buff,
                       uint32_t& p_result_crc)
{
    unsigned long calculate_crc = 0;
    unsigned int size_read = 0;

    // Determine the size of the file to preallocate the buffer
    p_input_stream.seekg(0, std::ios::end);
    std::streampos file_size = p_input_stream.tellg();
    p_input_stream.seekg(0, std::ios::beg);

    // If the file is of reasonable size, use a single buffer to avoid multiple
    // reads
    if (file_size > 0 &&
        file_size <
            100 * 1024 * 1024) // Limit to 100 Mo to avoid exhausting memory
    {
        // Resize the buffer to contain the whole file
        p_buff.resize(static_cast<size_t>(file_size));

        // Read the whole file in one go
        p_input_stream.read(p_buff.data(), file_size);
        size_read = static_cast<unsigned int>(p_input_stream.gcount());

        if (size_read > 0)
            calculate_crc =
                crc32(calculate_crc,
                      reinterpret_cast<const unsigned char*>(p_buff.data()),
                      size_read);

        // Reset the stream position
        p_input_stream.clear();
        p_input_stream.seekg(0, std::ios_base::beg);
    }
    else
    {
        // For large files, use the original approach with chunked reads
        do
        {
            p_input_stream.read(p_buff.data(), std::streamsize(p_buff.size()));
            size_read = static_cast<unsigned int>(p_input_stream.gcount());

            if (size_read > 0)
                calculate_crc =
                    crc32(calculate_crc,
                          reinterpret_cast<const unsigned char*>(p_buff.data()),
                          size_read);

        } while (size_read > 0);

        // Reset the stream position
        p_input_stream.clear();
        p_input_stream.seekg(0, std::ios_base::beg);
    }

    p_result_crc = static_cast<uint32_t>(calculate_crc);
}

// *************************************************************************
//! \brief PIMPL implementation
// *************************************************************************
struct Zipper::Impl
{
    Zipper& m_outer;
    zipFile m_zip_file = nullptr;
    ourmemory_t m_zip_memory;
    zlib_filefunc_def m_file_func;
    std::error_code& m_error_code;
    std::vector<char> m_buffer;

    // -------------------------------------------------------------------------
    Impl(Zipper& p_outer, std::error_code& p_error_code)
        : m_outer(p_outer),
          m_zip_memory(),
          m_file_func(),
          m_error_code(p_error_code),
          m_buffer(ZIPPER_WRITE_BUFFER_SIZE)
    {
        memset(&m_zip_memory, 0, sizeof(m_zip_memory));
        memset(&m_file_func, 0, sizeof(m_file_func));
    }

    // -------------------------------------------------------------------------
    ~Impl()
    {
        close();
    }

    // -------------------------------------------------------------------------
    bool initFile(const std::string& p_filename, Zipper::OpenFlags p_flags)
    {
#if defined(_WIN32)
        zlib_filefunc64_def ffunc = {0};
#endif

        int mode = 0;

        /* open the zip file for output */
        if (Path::exist(p_filename))
        {
            if (!Path::isFile(p_filename))
            {
                m_error_code = make_error_code(ZipperError::OPENING_ERROR,
                                               "Is a directory");
                return false;
            }
            if (p_flags == Zipper::OpenFlags::Overwrite)
            {
                Path::remove(p_filename);
                mode = APPEND_STATUS_CREATE;
            }
            else
            {
                mode = APPEND_STATUS_ADDINZIP;
            }
        }
        else
        {
            mode = APPEND_STATUS_CREATE;
        }

#if defined(_WIN32)
        fill_win32_filefunc64A(&ffunc);
        m_zip_file = zipOpen2_64(p_filename.c_str(), mode, nullptr, &ffunc);
#else
        m_zip_file = zipOpen64(p_filename.c_str(), mode);
#endif

        if (m_zip_file != nullptr)
            return true;
        m_error_code =
            make_error_code(ZipperError::OPENING_ERROR, strerror(errno));
        return false;
    }

    // -------------------------------------------------------------------------
    bool initWithStream(std::iostream& p_stream)
    {
        m_zip_memory.grow = 1;

        // Determine the size of the file to preallocate the buffer
        p_stream.seekg(0, std::ios::end);
        std::streampos s = p_stream.tellg();
        if (s < 0)
        {
            m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                           "Invalid stream provided");
            return false;
        }
        size_t size = static_cast<size_t>(s);
        p_stream.seekg(0);

        if (m_zip_memory.base != nullptr)
        {
            free(m_zip_memory.base);
            memset(&m_zip_memory, 0, sizeof(m_zip_memory));
        }

        // Allocate memory directly
        m_zip_memory.base = reinterpret_cast<char*>(malloc(size));
        if (m_zip_memory.base == nullptr)
        {
            m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                           "Failed to allocate memory");
            return false;
        }

        // Use the member buffer for reading
        constexpr size_t CHUNK_SIZE = 1024 * 1024; // 1 Mo per chunk
        if (m_buffer.size() < std::min(CHUNK_SIZE, size))
        {
            m_buffer.resize(std::min(CHUNK_SIZE, size));
        }

        char* dest = m_zip_memory.base;
        size_t remaining = size;

        // Read by chunks to avoid memory issues with large files
        while (remaining > 0 && p_stream.good())
        {
            size_t to_read = std::min(m_buffer.size(), remaining);
            p_stream.read(m_buffer.data(), std::streamsize(to_read));
            size_t actually_read = static_cast<size_t>(p_stream.gcount());

            if (actually_read == 0)
                break;

            memcpy(dest, m_buffer.data(), actually_read);
            dest += actually_read;
            remaining -= actually_read;
        }

        // If we couldn't read all the content, adjust the size
        if (remaining > 0)
        {
            size_t actual_size = size - remaining;
            // Reallocate to free unused memory
            m_zip_memory.base = reinterpret_cast<char*>(
                realloc(m_zip_memory.base, actual_size));
            size = actual_size;
        }

        m_zip_memory.size = static_cast<uint32_t>(size);

        fill_memory_filefunc(&m_file_func, &m_zip_memory);

        return initMemory(size > 0 ? APPEND_STATUS_CREATE
                                   : APPEND_STATUS_ADDINZIP,
                          m_file_func);
    }

    // -------------------------------------------------------------------------
    bool initWithVector(std::vector<unsigned char>& p_buffer)
    {
        m_zip_memory.grow = 1;

        if (!p_buffer.empty())
        {
            // Free existing memory if it exists
            if (m_zip_memory.base != nullptr)
            {
                free(m_zip_memory.base);
                memset(&m_zip_memory, 0, sizeof(m_zip_memory));
            }

            // Allocate memory directly with the correct size
            m_zip_memory.base =
                reinterpret_cast<char*>(malloc(p_buffer.size()));
            if (m_zip_memory.base == nullptr)
            {
                memset(&m_zip_memory, 0, sizeof(m_zip_memory));
                m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                               "Failed to allocate memory");
                return false;
            }

            // Use memcpy which is generally more optimized for large copies
            memcpy(m_zip_memory.base, p_buffer.data(), p_buffer.size());
            m_zip_memory.size = static_cast<uint32_t>(p_buffer.size());
        }
        else // Handle empty vector case
        {
            if (m_zip_memory.base != nullptr)
            {
                free(m_zip_memory.base);
                m_zip_memory.base = nullptr;
            }
            m_zip_memory.size = 0;
        }

        fill_memory_filefunc(&m_file_func, &m_zip_memory);
        return initMemory(p_buffer.empty() ? APPEND_STATUS_CREATE
                                           : APPEND_STATUS_ADDINZIP,
                          m_file_func);
    }

    // -------------------------------------------------------------------------
    bool initMemory(int p_mode, zlib_filefunc_def& p_file_func)
    {
        m_zip_file = zipOpen3("__notused__", p_mode, 0, 0, &p_file_func);
        if (m_zip_file != nullptr)
            return true;
        m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                       "zipOpen3 failed for memory mode");
        return false;
    }

    // -------------------------------------------------------------------------
    bool add(std::istream& p_input_stream,
             const std::tm& p_timestamp,
             const std::string& p_name_in_zip,
             const std::string& p_password,
             int p_flags)
    {
        if (!m_zip_file)
        {
            m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                           "Zip archive not open");
            return false;
        }

        // Ensure buffer exists and has the standard size.
        // Should be guaranteed by constructor, but check doesn't hurt.
        if (m_buffer.size() != ZIPPER_WRITE_BUFFER_SIZE)
        {
            m_buffer.resize(ZIPPER_WRITE_BUFFER_SIZE);
        }

        int compress_level = 5; // Zipper::ZipFlags::Medium
        int err = ZIP_OK;
        uint32_t crc_file = 0; // Calculated only if password is used

        zip_fileinfo zi;
        memset(&zi, 0, sizeof(zi)); // Zero out the structure first
        zi.dos_date = 0;            // if dos_date == 0, tmz_date is used
        zi.internal_fa = 0;         // internal file attributes
        zi.external_fa = 0;         // external file attributes
        zi.tmz_date.tm_sec = static_cast<uInt>(p_timestamp.tm_sec);
        zi.tmz_date.tm_min = static_cast<uInt>(p_timestamp.tm_min);
        zi.tmz_date.tm_hour = static_cast<uInt>(p_timestamp.tm_hour);
        zi.tmz_date.tm_mday = static_cast<uInt>(p_timestamp.tm_mday);
        zi.tmz_date.tm_mon = static_cast<uInt>(p_timestamp.tm_mon);
        zi.tmz_date.tm_year = static_cast<uInt>(p_timestamp.tm_year);

        if (p_name_in_zip.empty())
        {
            m_error_code = make_error_code(ZipperError::NO_ENTRY,
                                           "Entry name cannot be empty");
            return false;
        }

        std::string canon_name_in_zip = Path::canonicalPath(p_name_in_zip);

        // Prevent Zip Slip attack (See ticket #33)
        if (canon_name_in_zip.find_first_of("..") == 0u)
        {
            std::stringstream str;
            str << "Security error: forbidden insertion of '" << p_name_in_zip
                << "' (canonical: '" << canon_name_in_zip
                << "') to prevent possible Zip Slip attack";
            m_error_code =
                make_error_code(ZipperError::SECURITY_ERROR, str.str());
            return false;
        }

        // Determine compression level from flags (mask out hierarchy flag)
        int compression_flag =
            p_flags & (~static_cast<int>(Zipper::ZipFlags::SaveHierarchy));
        switch (compression_flag)
        {
            case Zipper::ZipFlags::Store:
                compress_level = 0;
                break;
            case Zipper::ZipFlags::Faster:
                compress_level = 1;
                break;
            case Zipper::ZipFlags::Better:
                compress_level = 9;
                break;
            case Zipper::ZipFlags::Medium:
                compress_level = 5;
                break;
            default: {
                std::stringstream str;
                str << "Invalid compression level flag: " << p_flags;
                m_error_code =
                    make_error_code(ZipperError::INTERNAL_ERROR, str.str());
                return false;
            }
        }

        bool zip64 = Path::isLargeFile(p_input_stream);
        if (p_password.empty())
        {
            err = zipOpenNewFileInZip64(m_zip_file,
                                        canon_name_in_zip.c_str(),
                                        &zi,
                                        nullptr, // extrafield_local
                                        0,       // size_extrafield_local
                                        nullptr, // extrafield_global
                                        0,       // size_extrafield_global
                                        nullptr, // comment
                                        (compress_level != 0) ? Z_DEFLATED : 0,
                                        compress_level,
                                        zip64);
        }
        else
        {
            // Calculate CRC32 first, as it's needed for encryption header
            // This reads the entire stream.
            getFileCrc(p_input_stream, m_buffer, crc_file);
            err =
                zipOpenNewFileInZip3_64(m_zip_file,
                                        canon_name_in_zip.c_str(),
                                        &zi,
                                        nullptr, // extrafield_local
                                        0,       // size_extrafield_local
                                        nullptr, // extrafield_global
                                        0,       // size_extrafield_global
                                        nullptr, // comment
                                        (compress_level != 0) ? Z_DEFLATED : 0,
                                        compress_level,
                                        0, // raw == 1 means no compression,
                                           // AES encryption needs compression
                                        /* Following are crypto parameters */
                                        -MAX_WBITS,         // windowBits
                                        DEF_MEM_LEVEL,      // memLevel
                                        Z_DEFAULT_STRATEGY, // strategy
                                        p_password.c_str(), // password
                                        crc_file,           // crcForCrypting
                                        zip64);
        }

        if (err != ZIP_OK)
        {
            std::stringstream str;
            str << "Error opening '" << p_name_in_zip
                << "' in zip file, error code: " << err;
            m_error_code =
                make_error_code(ZipperError::INTERNAL_ERROR, str.str());
            return false;
        }

        // Read from input stream and write to zip file chunk by chunk
        size_t size_read = 0;
        do
        {
            p_input_stream.read(m_buffer.data(),
                                std::streamsize(m_buffer.size()));
            size_read = static_cast<size_t>(p_input_stream.gcount());

            if (size_read > 0)
            {
                err = zipWriteInFileInZip(m_zip_file,
                                          m_buffer.data(),
                                          static_cast<unsigned int>(size_read));
                if (err != ZIP_OK)
                {
                    std::stringstream str;
                    str << "Error writing '" << p_name_in_zip
                        << "' to zip file, error code: " << err;
                    m_error_code =
                        make_error_code(ZipperError::INTERNAL_ERROR, str.str());
                    // Don't close file in zip here, let the main close handle
                    // cleanup? Or try to close? Let's break and let the close
                    // file step handle it.
                    break;
                }
            }

            // Check stream state *after* trying to read
            else if (!p_input_stream.eof() && !p_input_stream.good())
            {
                err = ZIP_ERRNO;
                m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                               "Input stream read error");
                break;
            }
        } while (err == ZIP_OK && size_read > 0);

        // Close the current file entry in the zip archive
        int close_err = zipCloseFileInZip(m_zip_file);
        if (err == ZIP_OK &&
            close_err != ZIP_OK) // Report close error only if write loop was ok
        {
            std::stringstream str;
            str << "Error closing '" << p_name_in_zip
                << "' in zip file, error code: " << close_err;
            m_error_code =
                make_error_code(ZipperError::INTERNAL_ERROR, str.str());
            return false; // Return false on close error
        }

        // Return true only if both write loop and close entry succeeded
        return (err == ZIP_OK && close_err == ZIP_OK);
    }

    // -------------------------------------------------------------------------
    void close()
    {
        if (m_zip_file != nullptr)
        {
            zipClose(m_zip_file, nullptr);
            m_zip_file = nullptr;
        }

        if (m_zip_memory.base && m_zip_memory.limit > 0)
        {
            if (m_outer.m_using_vector)
            {
                m_outer.m_output_vector.resize(m_zip_memory.limit);
                m_outer.m_output_vector.assign(
                    m_zip_memory.base, m_zip_memory.base + m_zip_memory.limit);
            }
            else if (m_outer.m_using_stream)
            {
                m_outer.m_output_stream.write(
                    m_zip_memory.base, std::streamsize(m_zip_memory.limit));
            }
        }

        if (m_zip_memory.base != nullptr)
        {
            free(m_zip_memory.base);
            memset(&m_zip_memory, 0, sizeof(m_zip_memory));
        }

        // Free the memory of the buffer by resizing it to 0
        std::vector<char>().swap(m_buffer);
    }
};

// -------------------------------------------------------------------------
Zipper::Zipper(const std::string& p_zipname,
               const std::string& p_password,
               Zipper::OpenFlags p_flags)
    : m_output_stream(*(new std::stringstream())),
      m_output_vector(*(new std::vector<unsigned char>())),
      m_zip_name(p_zipname),
      m_password(p_password),
      m_using_vector(false),
      m_using_stream(false),
      m_impl(new Impl(*this, m_error_code))
{
    if (m_impl->initFile(p_zipname, p_flags))
    {
        // success
        m_open = true;
    }
    else if (m_impl->m_error_code)
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    else
    {
        // Other error (like dummy zip). Let it dummy
        m_open = true;
    }
}

// -------------------------------------------------------------------------
Zipper::Zipper(std::iostream& p_buffer, const std::string& p_password)
    : m_output_stream(p_buffer),
      m_output_vector(*(new std::vector<unsigned char>())),
      m_password(p_password),
      m_using_vector(false),
      m_using_stream(true),
      m_impl(new Impl(*this, m_error_code))
{
    if (!m_impl->initWithStream(p_buffer))
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    m_open = true;
}

// -------------------------------------------------------------------------
Zipper::Zipper(std::vector<unsigned char>& p_buffer,
               const std::string& p_password)
    : m_output_stream(*(new std::stringstream())),
      m_output_vector(p_buffer),
      m_password(p_password),
      m_using_vector(true),
      m_using_stream(false),
      m_impl(new Impl(*this, m_error_code))
{
    if (!m_impl->initWithVector(m_output_vector))
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    m_open = true;
}

// -------------------------------------------------------------------------
Zipper::~Zipper()
{
    close();
    release();
}

// -------------------------------------------------------------------------
void Zipper::close()
{
    if (m_open && m_impl)
    {
        m_impl->close();
    }
    m_open = false;
}

// -------------------------------------------------------------------------
void Zipper::release()
{
    if (!m_using_vector)
    {
        delete &m_output_vector;
    }
    if (!m_using_stream)
    {
        delete &m_output_stream;
    }
    if (m_impl != nullptr)
    {
        delete m_impl;
    }
}

// -------------------------------------------------------------------------
bool Zipper::add(std::istream& p_source,
                 const std::tm& p_timestamp,
                 const std::string& p_nameInZip,
                 ZipFlags p_flags)
{
    return m_impl->add(p_source, p_timestamp, p_nameInZip, m_password, p_flags);
}

// -------------------------------------------------------------------------
bool Zipper::add(std::istream& p_source,
                 const std::string& p_nameInZip,
                 ZipFlags p_flags)
{
    Timestamp time;
    return m_impl->add(
        p_source, time.timestamp, p_nameInZip, m_password, p_flags);
}

// -------------------------------------------------------------------------
bool Zipper::add(const std::string& p_file_or_folder_path,
                 Zipper::ZipFlags p_flags)
{
    // Check if open and impl exists
    if (!m_open || !m_impl)
    {
        if (!m_open)
        {
            m_error_code =
                make_error_code(ZipperError::OPENING_ERROR, "Zipper not open");
        }
        else
        {
            m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                           "Zipper not initialized");
        }
        return false;
    }

    bool overall_success = true;

    if (Path::isDir(p_file_or_folder_path))
    {
        // Get base folder path with separator for relative path calculation
        std::string folderBase =
            Path::folderNameWithSeparator(p_file_or_folder_path);
        std::vector<std::string> files =
            Path::filesFromDir(p_file_or_folder_path, true);
        for (const auto& filePath : files)
        {
            std::ifstream input(filePath.c_str(), std::ios::binary);
            if (!input.is_open())
            {
                m_error_code = make_error_code(ZipperError::OPENING_ERROR,
                                               "Cannot open file: " + filePath);
                // Mark failure but continue trying other files? Or break? Let's
                // continue.
                overall_success = false;
                continue;
            }
            std::string nameInZip;

            // If saving hierarchy and base folder is found, use relative path
            if (p_flags & Zipper::SaveHierarchy)
            {
                nameInZip = filePath;
            }
            else
            {
                nameInZip = Path::fileName(filePath);
            }

            // Call the stream-based add function
            Timestamp time(filePath);
            bool success = add(input, time.timestamp, nameInZip, p_flags);
            if (!success)
            {
                overall_success = false;
                // error_code should be set by the called add function
            }
        }
    }
    else // It's a single file
    {
        std::ifstream input(p_file_or_folder_path.c_str(), std::ios::binary);
        if (!input.is_open())
        {
            m_error_code =
                make_error_code(ZipperError::OPENING_ERROR,
                                "Cannot open file: " + p_file_or_folder_path);
            return false; // Fail immediately if single file cannot be opened
        }

        std::string nameInZip;

        // Check if hierarchy needs to be saved
        if (p_flags & Zipper::SaveHierarchy)
        {
            std::cout << "Saving hierarchy" << std::endl;
            // Use full path
            nameInZip = p_file_or_folder_path;
        }
        else
        {
            // Use just the filename
            std::cout << "Not saving hierarchy" << std::endl;
            nameInZip = Path::fileName(p_file_or_folder_path);
        }

        // Get timestamp for the file
        Timestamp time(p_file_or_folder_path);
        // Call the stream-based add function
        overall_success = add(input, time.timestamp, nameInZip, p_flags);
    }

    return overall_success;
}

// -------------------------------------------------------------------------
bool Zipper::open(Zipper::OpenFlags p_flags)
{
    if (m_impl == nullptr)
    {
        m_error_code =
            make_error_code(ZipperError::INTERNAL_ERROR,
                            "Zipper implementation not available for open");
        return false;
    }

    // If already open, do nothing and return success
    if (m_open)
    {
        return true;
    }

    bool success = false;
    if (m_using_vector)
    {
        success = m_impl->initWithVector(m_output_vector);
    }
    else if (m_using_stream)
    {
        success = m_impl->initWithStream(m_output_stream);
    }
    else
    {
        success = m_impl->initFile(m_zip_name, p_flags);
    }

    if (success)
    {
        m_open = true;
        m_error_code = {};
    }
    return success;
}

// -----------------------------------------------------------------------------
std::error_code const& Zipper::error() const
{
    return m_error_code;
}

// -----------------------------------------------------------------------------
bool Zipper::isOpen() const
{
    return m_open;
}

} // namespace zipper
