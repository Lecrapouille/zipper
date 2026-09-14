//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#include "Zipper/Zipper.hpp"
#include "utils/FilePath.hpp"
#include "utils/OS.hpp"
#include "utils/Path.hpp"
#include "utils/Timestamp.hpp"

#include "external/minizip/ioapi_mem.h"
#include "external/minizip/zip.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifndef ZIPPER_WRITE_BUFFER_SIZE
#    define ZIPPER_WRITE_BUFFER_SIZE (65536u)
#endif

namespace zipper
{

enum class ZipperError
{
    //! No error
    NO_ERROR_ZIPPER = 0,
    //! Error when accessing to a info entry
    BAD_ENTRY,
    //! Error when opening a zip file
    OPENING_ERROR,
    //! Error inside this library
    INTERNAL_ERROR,
    //! Zip slip vulnerability
    SECURITY_ERROR,
    //! Error when adding a file to a zip file
    ADDING_ERROR
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

    virtual std::string message(int /*p_error*/) const override
    {
        return custom_message;
    }

    std::string custom_message;
};

// -----------------------------------------------------------------------------
static ZipperErrorCategory theZipperErrorCategory;

// -----------------------------------------------------------------------------
static std::error_code make_error_code(ZipperError p_error,
                                       std::string const& p_message)
{
    theZipperErrorCategory.custom_message = p_message;
    return { static_cast<int>(p_error), theZipperErrorCategory };
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

    // Chunked reads
    do
    {
        p_input_stream.read(p_buff.data(), std::streamsize(p_buff.size()));
        size_read = static_cast<unsigned int>(p_input_stream.gcount());
        if (size_read > 0)
        {
            calculate_crc =
                crc32(calculate_crc,
                      reinterpret_cast<const unsigned char*>(p_buff.data()),
                      size_read);
        }
    } while (size_read > 0);
    p_result_crc = static_cast<uint32_t>(calculate_crc);

    // Reset the stream position
    p_input_stream.clear();
    p_input_stream.seekg(0, std::ios_base::beg);
}

// Unix ZIP convention: POSIX mode bits (\c chmod) are stored in the upper 16
// bits of the ZIP central directory \c external_fa word.
#if !defined(_WIN32)
static uint32_t zip_unix_external_attributes(char const* p_disk_path)
{
    STAT st{};
    if (::stat(p_disk_path, &st) != 0)
    {
        return 0;
    }
    return (static_cast<uint32_t>(st.st_mode) & 07777U) << 16U;
}
#endif

// *************************************************************************
//! \brief minizip I/O backend writing straight into the caller-owned
//! std::vector or std::iostream.
//!
//! The historical implementation compressed into a private malloc'd buffer
//! (ourmemory_t) and only copied it into the user reference inside close().
//! This sink removes that intermediate copy: every byte minizip writes
//! (including the central directory produced by zipClose()) lands directly in
//! the user's container, so the reference mirrors the archive as it grows.
//!
//! \note The archive only becomes a *parsable* ZIP once the End Of Central
//! Directory record has been written, i.e. after close() or flush(). Before
//! that the reference holds valid raw bytes but no directory index.
// *************************************************************************
struct OutputSink
{
    enum class Kind
    {
        None,
        Vector,
        Stream
    };

    Kind kind = Kind::None;
    std::vector<unsigned char>* vec = nullptr;
    std::iostream* stream = nullptr;
    uint64_t cursor = 0; //!< Current read/write offset.

    void setVector(std::vector<unsigned char>* p_vec)
    {
        kind = Kind::Vector;
        vec = p_vec;
        stream = nullptr;
        cursor = 0;
    }

    void setStream(std::iostream* p_stream)
    {
        kind = Kind::Stream;
        stream = p_stream;
        vec = nullptr;
        cursor = 0;
    }

    //! \brief Logical amount of data currently stored (the "end" position).
    uint64_t size()
    {
        if (kind == Kind::Vector)
        {
            return vec->size();
        }
        if (kind == Kind::Stream)
        {
            stream->clear();
            stream->seekg(0, std::ios::end);
            const std::streampos end = stream->tellg();
            return (end > 0) ? static_cast<uint64_t>(end) : 0u;
        }
        return 0u;
    }

    //! \brief Drop any existing content (used for Overwrite / CREATE).
    void truncate()
    {
        cursor = 0;
        if (kind == Kind::Vector)
        {
            vec->clear();
        }
        else if (kind == Kind::Stream)
        {
            stream->clear();
            // Best effort: reset a std::stringstream, otherwise rewind.
            auto* ss = dynamic_cast<std::stringstream*>(stream);
            if (ss != nullptr)
            {
                ss->str(std::string());
            }
            stream->seekp(0, std::ios::beg);
        }
    }

    //! \brief Trim any trailing bytes beyond the current cursor. Called right
    //! after the End Of Central Directory has been written so a re-finalized
    //! (appended) archive never keeps stale bytes from a previous, longer
    //! central directory.
    void truncateToCursor()
    {
        if (kind == Kind::Vector)
        {
            if (cursor < vec->size())
            {
                vec->resize(static_cast<size_t>(cursor));
            }
        }
        else if (kind == Kind::Stream)
        {
            auto* ss = dynamic_cast<std::stringstream*>(stream);
            if (ss != nullptr)
            {
                const std::string all = ss->str();
                if (cursor < all.size())
                {
                    ss->str(all.substr(0, static_cast<size_t>(cursor)));
                }
            }
        }
    }

    void flush()
    {
        if (kind == Kind::Stream)
        {
            stream->flush();
        }
    }
};

// -----------------------------------------------------------------------------
static voidpf ZCALLBACK sink_open(voidpf opaque,
                                  const char* /*filename*/,
                                  int mode)
{
    OutputSink* sink = static_cast<OutputSink*>(opaque);
    if (sink == nullptr)
    {
        return nullptr;
    }
    if ((mode & ZLIB_FILEFUNC_MODE_CREATE) != 0)
    {
        sink->truncate();
    }
    sink->cursor = 0;
    return sink;
}

// -----------------------------------------------------------------------------
static uint32_t ZCALLBACK sink_read(voidpf /*opaque*/,
                                    voidpf stream,
                                    void* buf,
                                    uint32_t size)
{
    OutputSink* sink = static_cast<OutputSink*>(stream);
    const uint64_t available = sink->size();
    if (sink->cursor >= available)
    {
        return 0;
    }
    uint32_t to_read = size;
    if (sink->cursor + to_read > available)
    {
        to_read = static_cast<uint32_t>(available - sink->cursor);
    }

    if (sink->kind == OutputSink::Kind::Vector)
    {
        std::memcpy(buf,
                    sink->vec->data() + sink->cursor,
                    static_cast<size_t>(to_read));
    }
    else
    {
        sink->stream->clear();
        sink->stream->seekg(static_cast<std::streamoff>(sink->cursor),
                            std::ios::beg);
        sink->stream->read(static_cast<char*>(buf),
                           static_cast<std::streamsize>(to_read));
        to_read = static_cast<uint32_t>(sink->stream->gcount());
    }
    sink->cursor += to_read;
    return to_read;
}

// -----------------------------------------------------------------------------
static uint32_t ZCALLBACK sink_write(voidpf /*opaque*/,
                                     voidpf stream,
                                     const void* buf,
                                     uint32_t size)
{
    OutputSink* sink = static_cast<OutputSink*>(stream);

    if (sink->kind == OutputSink::Kind::Vector)
    {
        const uint64_t needed = sink->cursor + size;
        if (needed > sink->vec->size())
        {
            sink->vec->resize(static_cast<size_t>(needed));
        }
        std::memcpy(
            sink->vec->data() + sink->cursor, buf, static_cast<size_t>(size));
    }
    else
    {
        sink->stream->clear();
        sink->stream->seekp(static_cast<std::streamoff>(sink->cursor),
                            std::ios::beg);
        sink->stream->write(static_cast<const char*>(buf),
                            static_cast<std::streamsize>(size));
        if (!sink->stream->good())
        {
            return 0;
        }
    }
    sink->cursor += size;
    return size;
}

// -----------------------------------------------------------------------------
static long ZCALLBACK sink_tell(voidpf /*opaque*/, voidpf stream)
{
    OutputSink* sink = static_cast<OutputSink*>(stream);
    return static_cast<long>(sink->cursor);
}

// -----------------------------------------------------------------------------
static long ZCALLBACK sink_seek(voidpf /*opaque*/,
                                voidpf stream,
                                uint32_t offset,
                                int origin)
{
    OutputSink* sink = static_cast<OutputSink*>(stream);
    uint64_t new_pos = 0;
    switch (origin)
    {
        case ZLIB_FILEFUNC_SEEK_CUR:
            new_pos = sink->cursor + offset;
            break;
        case ZLIB_FILEFUNC_SEEK_END:
            new_pos = sink->size() + offset;
            break;
        case ZLIB_FILEFUNC_SEEK_SET:
            new_pos = offset;
            break;
        default:
            return -1;
    }
    sink->cursor = new_pos;
    return 0;
}

// -----------------------------------------------------------------------------
static int ZCALLBACK sink_close(voidpf /*opaque*/, voidpf /*stream*/)
{
    return 0;
}

// -----------------------------------------------------------------------------
static int ZCALLBACK sink_error(voidpf /*opaque*/, voidpf /*stream*/)
{
    return 0;
}

// -----------------------------------------------------------------------------
static void fill_output_sink_filefunc(zlib_filefunc_def* p_filefunc,
                                      OutputSink* p_sink)
{
    p_filefunc->zopen_file = sink_open;
    p_filefunc->zopendisk_file = nullptr;
    p_filefunc->zread_file = sink_read;
    p_filefunc->zwrite_file = sink_write;
    p_filefunc->ztell_file = sink_tell;
    p_filefunc->zseek_file = sink_seek;
    p_filefunc->zclose_file = sink_close;
    p_filefunc->zerror_file = sink_error;
    p_filefunc->opaque = p_sink;
}

// *************************************************************************
//! \brief PIMPL implementation
// *************************************************************************
struct Zipper::Impl
{
    Zipper& m_outer;
    zipFile m_zip_handler = nullptr;
    //! \brief I/O backend writing directly into the caller's vector/stream.
    OutputSink m_sink;
    zlib_filefunc_def m_file_func;
    std::error_code& m_error_code;
    std::vector<char> m_buffer;
    Progress m_progress;
    ProgressCallback m_progress_callback;
    //! \brief True when compressing into a std::vector/std::iostream (memory),
    //! false for a disk file.
    bool m_use_sink = false;

    // -------------------------------------------------------------------------
    Impl(Zipper& p_outer, std::error_code& p_error_code)
        : m_outer(p_outer),
          m_file_func(),
          m_error_code(p_error_code),
          m_buffer(ZIPPER_WRITE_BUFFER_SIZE)
    {
        memset(&m_file_func, 0, sizeof(m_file_func));
    }

    // -------------------------------------------------------------------------
    ~Impl()
    {
        close();
    }

    // -------------------------------------------------------------------------
    bool initFile(const std::filesystem::path& p_filename,
                  Zipper::OpenFlags p_flags)
    {
        // Set the minizip opening mode
        int mode = 0;
        if (p_flags == Zipper::OpenFlags::Overwrite)
        {
            mode = APPEND_STATUS_CREATE;
        }
        else
        {
            mode = APPEND_STATUS_ADDINZIP;
        }

        // Open the zip file. On Windows always use the wide API so Unicode
        // paths (and UTF-8 std::string converted via utf8ToPath) work.
#if defined(_WIN32)
        zlib_filefunc64_def ffunc = { 0 };
        fill_win32_filefunc64W(&ffunc);
        m_zip_handler = zipOpen2_64(p_filename.c_str(), mode, nullptr, &ffunc);
#else
        m_zip_handler = zipOpen64(p_filename.c_str(), mode);
#endif

        // If the zip file is not opened, return an custom error message
        if (m_zip_handler == nullptr)
        {
            const std::string name = pathToUtf8(p_filename);
            std::stringstream str;
            str << "Failed opening zip file '" << name << "'. Reason: ";

            std::error_code fs_ec;
            if (std::filesystem::is_directory(p_filename, fs_ec))
            {
                str << "Is a directory";
            }
            else if ((errno == EINVAL) || p_filename.extension() != ".zip")
            {
                str << "Not a zip file";
            }
            else
            {
                str << nativeErrorMessage();
            }

            m_error_code =
                make_error_code(ZipperError::OPENING_ERROR, str.str());
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool initWithStream(std::iostream& p_stream)
    {
        // Wire minizip directly onto the caller's stream: no intermediate
        // buffer, so every written byte lands in p_stream as the archive grows.
        m_sink.setStream(&p_stream);
        m_use_sink = true;

        const uint64_t existing = m_sink.size();

        // Overwrite (or empty stream): create from scratch. Append: minizip
        // reads back the central directory already present in the stream.
        const int mode = ((existing == 0) || (m_outer.m_open_flags ==
                                              Zipper::OpenFlags::Overwrite))
                             ? APPEND_STATUS_CREATE
                             : APPEND_STATUS_ADDINZIP;

        fill_output_sink_filefunc(&m_file_func, &m_sink);
        return initMemory(mode, m_file_func);
    }

    // -------------------------------------------------------------------------
    bool initWithVector(std::vector<unsigned char>& p_buffer)
    {
        // Wire minizip directly onto the caller's vector.
        m_sink.setVector(&p_buffer);
        m_use_sink = true;

        // Historical semantics for vectors: an empty vector starts a new
        // archive, a non-empty vector is appended to (the open flag is not
        // consulted here, matching the pre-refactor behaviour).
        const int mode =
            p_buffer.empty() ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP;

        fill_output_sink_filefunc(&m_file_func, &m_sink);
        return initMemory(mode, m_file_func);
    }

    // -------------------------------------------------------------------------
    bool initMemory(int p_mode, zlib_filefunc_def& p_file_func)
    {
        m_zip_handler = zipOpen3("__notused__", p_mode, 0, 0, &p_file_func);
        if (m_zip_handler == nullptr)
        {
            m_error_code = make_error_code(ZipperError::OPENING_ERROR,
                                           "Failed opening zip memory");
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool add(std::istream& p_input_stream,
             const std::tm& p_timestamp,
             const std::string& p_name_in_zip,
             const std::string& p_password,
             int p_flags,
             uint32_t p_unix_external_attrs = 0)
    {
        if (!m_zip_handler)
        {
            m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                           "Zip archive is not opened");
            return false;
        }

        // The internal work buffer is released by close(); make sure it is
        // available again if the zipper is reused after a close()/flush().
        if (m_buffer.empty())
        {
            m_buffer.resize(ZIPPER_WRITE_BUFFER_SIZE);
        }

        if (m_progress_callback)
        {
            m_progress.status = Progress::Status::InProgress;
            m_progress.current_file = p_name_in_zip;
            m_progress_callback(m_progress);
        }

        int compress_level = 5; // Zipper::ZipFlags::Medium
        int err = ZIP_OK;
        uint32_t crc_file = 0; // Calculated only if password is used

        zip_fileinfo zi;
        memset(&zi, 0, sizeof(zi)); // Zero out the structure first
        zi.dos_date = 0;            // if dos_date == 0, tmz_date is used
        zi.internal_fa = 0;         // internal file attributes
        zi.external_fa = static_cast<uLong>(p_unix_external_attrs);
        zi.tmz_date.tm_sec = static_cast<uInt>(p_timestamp.tm_sec);
        zi.tmz_date.tm_min = static_cast<uInt>(p_timestamp.tm_min);
        zi.tmz_date.tm_hour = static_cast<uInt>(p_timestamp.tm_hour);
        zi.tmz_date.tm_mday = static_cast<uInt>(p_timestamp.tm_mday);
        zi.tmz_date.tm_mon = static_cast<uInt>(p_timestamp.tm_mon);
        zi.tmz_date.tm_year = static_cast<uInt>(p_timestamp.tm_year);

        // Check if the entry name is valid to prevent security issues
        std::string canon_name_in_zip = Path::normalize(p_name_in_zip);
        Path::InvalidEntryReason reason = Path::isValidEntry(canon_name_in_zip);
        if ((reason != Path::InvalidEntryReason::VALID_ENTRY) &&
            (reason != Path::InvalidEntryReason::ABSOLUTE_PATH))
        {
            m_error_code = make_error_code(
                ZipperError::SECURITY_ERROR,
                "Zip entry name '" + p_name_in_zip + "' is invalid because " +
                    Path::getInvalidEntryReason(reason));
            return false;
        }

        // Silently remove the absolute path from the entry name
        if (reason == Path::InvalidEntryReason::ABSOLUTE_PATH)
        {
            canon_name_in_zip = canon_name_in_zip.substr(
                Path::root(canon_name_in_zip).length());
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
            default:
            {
                std::stringstream str;
                str << "Invalid compression level flag: " << p_flags;
                m_error_code =
                    make_error_code(ZipperError::BAD_ENTRY, str.str());
                return false;
            }
        }

        bool zip64 = Path::isLargeFile(p_input_stream);
        if (p_password.empty())
        {
            err = zipOpenNewFileInZip64(m_zip_handler,
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
                zipOpenNewFileInZip3_64(m_zip_handler,
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
            str << "Failed opening file '" << p_name_in_zip;
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
                err = zipWriteInFileInZip(m_zip_handler,
                                          m_buffer.data(),
                                          static_cast<unsigned int>(size_read));
                if (err == ZIP_OK)
                {
                    if (m_progress_callback)
                    {
                        m_progress.bytes_processed += size_read;
                        m_progress_callback(m_progress);
                    }
                }
                else
                {
                    std::stringstream str;
                    str << "Failed writing '" << p_name_in_zip << "'";
                    m_error_code =
                        make_error_code(ZipperError::INTERNAL_ERROR, str.str());
                    // Don't close file in zip here, let the main close handle
                    // cleanup? Or try to close? Let's break and let the close
                    // file step handle it.
                    break;
                }
            }

            // Check stream state
            else if (!p_input_stream.eof() && !p_input_stream.good())
            {
                err = ZIP_ERRNO;
                m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                               "Failed reading input stream");
                break;
            }
        } while (err == ZIP_OK && size_read > 0);

        // Close the current file entry in the zip archive
        int close_err = zipCloseFileInZip(m_zip_handler);
        if ((err == ZIP_OK) && (close_err != ZIP_OK))
        {
            std::stringstream str;
            str << "Failed closing file '" << p_name_in_zip << "'";
            m_error_code =
                make_error_code(ZipperError::INTERNAL_ERROR, str.str());
            return false; // Return false on close error
        }

        // Return true only if both write loop and close entry succeeded
        bool result = ((err == ZIP_OK) && (close_err == ZIP_OK));

        // Update progress callback
        if (m_progress_callback)
        {
            m_progress.files_compressed += result;
            m_progress_callback(m_progress);
        }

        return result;
    }

    // -------------------------------------------------------------------------
    //! \brief Write the central directory + End Of Central Directory record,
    //! turning whatever has been written so far into a valid, parsable ZIP.
    //! On success the minizip handle is consumed (set to null).
    bool finalizeArchive()
    {
        if (m_zip_handler == nullptr)
        {
            return true; // Nothing open: already finalized.
        }

        const int err = zipClose(m_zip_handler, nullptr);
        m_zip_handler = nullptr;
        if (m_use_sink)
        {
            m_sink.truncateToCursor();
        }
        m_sink.flush();

        if (err != ZIP_OK)
        {
            m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                           "Failed finalizing zip archive");
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    //! \brief Reopen the just-finalized archive in append mode so the caller
    //! can keep adding entries after a flush().
    bool reopenAppend()
    {
        if (m_use_sink)
        {
            // The sink already holds a valid archive (EOCD written); minizip
            // reads its central directory back and appends to it.
            return initMemory(APPEND_STATUS_ADDINZIP, m_file_func);
        }
        return initFile(m_outer.m_zip_name, Zipper::OpenFlags::Append);
    }

    // -------------------------------------------------------------------------
    //! \brief Finalize the archive then reopen it for appending. After this
    //! call the referenced vector/stream is a valid ZIP and the zipper is
    //! still usable for further add() calls.
    bool flush()
    {
        if (m_zip_handler == nullptr)
        {
            return true;
        }
        if (!finalizeArchive())
        {
            return false;
        }
        return reopenAppend();
    }

    // -------------------------------------------------------------------------
    //! \brief Called after a successful add() to honour the auto-flush option.
    bool maybeAutoFlush()
    {
        if (!m_outer.m_auto_flush)
        {
            return true;
        }
        return flush();
    }

    // -------------------------------------------------------------------------
    void close()
    {
        // Finalize the archive (writes central directory + EOCD straight into
        // the sink/file). No copy step is required anymore.
        finalizeArchive();

        // Free the memory of the internal buffer.
        if (!m_buffer.empty())
        {
            std::vector<char>().swap(m_buffer);
        }
    }
};

// -------------------------------------------------------------------------
Zipper::Zipper() : m_impl(nullptr) {}

// -------------------------------------------------------------------------
Zipper::Zipper(const std::string& p_zipname,
               const std::string& p_password,
               Zipper::OpenFlags p_open_flags)
    : Zipper(utf8ToPath(p_zipname), p_password, p_open_flags)
{
}

// -------------------------------------------------------------------------
Zipper::Zipper(const std::filesystem::path& p_zipname,
               const std::string& p_password,
               Zipper::OpenFlags p_open_flags)
    : m_zip_name(p_zipname),
      m_password(p_password),
      m_open_flags(p_open_flags),
      m_impl(std::make_unique<Impl>(*this, m_error_code))
{
    if (!reopen())
    {
        throw std::runtime_error(
            m_error_code ? m_error_code.message()
                         : "Zipper initialization with file failed");
    }
}

// -------------------------------------------------------------------------
Zipper::Zipper(std::iostream& p_buffer,
               const std::string& p_password,
               Zipper::OpenFlags p_open_flags)
    : m_output_stream(&p_buffer),
      m_password(p_password),
      m_open_flags(p_open_flags),
      m_impl(std::make_unique<Impl>(*this, m_error_code))
{
    if (!reopen())
    {
        throw std::runtime_error(
            m_error_code ? m_error_code.message()
                         : "Zipper initialization with stream failed");
    }
}

// -------------------------------------------------------------------------
Zipper::Zipper(std::iostream& p_buffer, const std::string& p_password)
    : m_output_stream(&p_buffer),
      m_password(p_password),
      m_open_flags(OpenFlags::Overwrite),
      m_impl(std::make_unique<Impl>(*this, m_error_code))
{
    if (!reopen())
    {
        throw std::runtime_error(
            m_error_code ? m_error_code.message()
                         : "Zipper initialization with stream failed");
    }
}

// -------------------------------------------------------------------------
Zipper::Zipper(std::vector<unsigned char>& p_buffer,
               const std::string& p_password)
    : m_output_vector(&p_buffer),
      m_password(p_password),
      m_open_flags(OpenFlags::Overwrite),
      m_impl(std::make_unique<Impl>(*this, m_error_code))
{
    if (!reopen())
    {
        throw std::runtime_error(
            m_error_code ? m_error_code.message()
                         : "Zipper initialization with vector failed");
    }
}

// -------------------------------------------------------------------------
Zipper::~Zipper()
{
    close();
}

// -------------------------------------------------------------------------
void Zipper::close()
{
    if (m_open && m_impl)
    {
        m_impl->close();
    }
    m_open = false;
    m_error_code.clear();
}

// -------------------------------------------------------------------------
bool Zipper::setProgressCallback(ProgressCallback callback)
{
    if (m_impl)
    {
        m_impl->m_progress_callback = std::move(callback);
        return true;
    }
    return false;
}

// -------------------------------------------------------------------------
bool Zipper::flush()
{
    if (!checkValid())
        return false;

    if (!m_impl->flush())
    {
        return false;
    }
    m_error_code = {};
    return true;
}

// -------------------------------------------------------------------------
void Zipper::setAutoFlush(bool p_enable)
{
    m_auto_flush = p_enable;
}

// -------------------------------------------------------------------------
bool Zipper::autoFlush() const
{
    return m_auto_flush;
}

// -------------------------------------------------------------------------
bool Zipper::add(std::istream& p_source,
                 const std::tm& p_timestamp,
                 const std::string& p_name_in_zip,
                 ZipFlags p_flags)
{
    if (!checkValid())
        return false;

    m_impl->m_progress.total_files = 1;
    m_impl->m_progress.bytes_processed = 0;
    m_impl->m_progress.files_compressed = 0;

    bool result =
        m_impl->add(p_source, p_timestamp, p_name_in_zip, m_password, p_flags);
    if (result)
    {
        result = m_impl->maybeAutoFlush();
    }
    return result;
}

// -------------------------------------------------------------------------
bool Zipper::add(std::istream& p_source,
                 const std::string& p_name_in_zip,
                 ZipFlags p_flags)
{
    if (!checkValid())
        return false;

    m_impl->m_progress.total_files = 1;
    m_impl->m_progress.bytes_processed = 0;
    m_impl->m_progress.files_compressed = 0;

    Timestamp time;
    bool result = m_impl->add(
        p_source, time.timestamp, p_name_in_zip, m_password, p_flags);
    if (result)
    {
        result = m_impl->maybeAutoFlush();
    }
    return result;
}

// -------------------------------------------------------------------------
bool Zipper::add(const std::string& p_file_or_folder_path,
                 Zipper::ZipFlags p_flags)
{
    return add(utf8ToPath(p_file_or_folder_path), p_flags);
}

// -------------------------------------------------------------------------
static std::string
zipEntryNameFromDiskPath(const std::filesystem::path& p_file,
                         const std::filesystem::path& p_added,
                         bool p_save_hierarchy)
{
    if (!p_save_hierarchy)
    {
        const auto u8 = p_file.filename().u8string();
        return { u8.begin(), u8.end() };
    }

    // Match the historical string API: "data/somefolder/" + test.txt
    // becomes "data/somefolder/test.txt" (the added folder is kept).
    auto folder = p_added.lexically_normal();
    if (!folder.has_filename())
    {
        folder = folder.parent_path();
    }

    const auto relative = p_file.lexically_relative(folder);
    const auto entry = (folder / relative).lexically_normal();
    const auto u8 = entry.generic_u8string();
    return { u8.begin(), u8.end() };
}

// -------------------------------------------------------------------------
bool Zipper::add(const std::filesystem::path& p_file_or_folder_path,
                 Zipper::ZipFlags p_flags)
{
    if (!checkValid())
        return false;

    // Clear previous error before attempting add operation
    m_error_code = {};
    bool overall_success = true;

    std::error_code fs_ec;
    if (std::filesystem::is_directory(p_file_or_folder_path, fs_ec))
    {
        std::vector<std::filesystem::path> files;
        try
        {
            auto it = std::filesystem::recursive_directory_iterator(
                p_file_or_folder_path,
                std::filesystem::directory_options::skip_permission_denied,
                fs_ec);
            if (fs_ec)
            {
                m_error_code = make_error_code(
                    ZipperError::ADDING_ERROR,
                    "Permission denied: '" + pathToUtf8(p_file_or_folder_path) +
                        "'");
                return false;
            }

            const auto end = std::filesystem::recursive_directory_iterator();
            for (; it != end; it.increment(fs_ec))
            {
                if (fs_ec)
                {
                    fs_ec.clear();
                    continue;
                }
                if (it->is_regular_file(fs_ec) && !fs_ec)
                {
                    files.push_back(it->path());
                }
            }

            m_impl->m_progress.total_files = files.size();
            m_impl->m_progress.total_bytes = 0;
            for (const auto& file : files)
            {
                std::ifstream input(file, std::ios::binary);
                if (input.is_open())
                {
                    input.seekg(0, std::ios::end);
                    m_impl->m_progress.total_bytes +=
                        static_cast<uint64_t>(input.tellg());
                    input.seekg(0, std::ios::beg);
                }
            }
        }
        catch (const std::exception& e)
        {
            const std::string what = e.what();
            m_error_code = make_error_code(
                ZipperError::ADDING_ERROR,
                (what.find("Permission") != std::string::npos)
                    ? ("Permission denied: '" +
                       pathToUtf8(p_file_or_folder_path) + "'")
                    : (std::string("Failed listing folder files: ") + what));
            return false;
        }

        if (files.empty())
        {
            std::error_code probe_ec;
            (void)std::filesystem::directory_iterator(p_file_or_folder_path,
                                                      probe_ec);
            if (probe_ec)
            {
                m_error_code = make_error_code(
                    ZipperError::ADDING_ERROR,
                    "Permission denied: '" + pathToUtf8(p_file_or_folder_path) +
                        "'");
                return false;
            }

            // TODO: add the directory itself to the zip
            return true;
        }

        const bool save_hierarchy =
            (p_flags & Zipper::SaveHierarchy) == Zipper::SaveHierarchy;

        for (const auto& file_path : files)
        {
            std::ifstream input(file_path, std::ios::binary);
            if (!input.is_open())
            {
                if (std::filesystem::is_regular_file(file_path, fs_ec))
                {
                    m_error_code = make_error_code(
                        ZipperError::ADDING_ERROR,
                        "Failed opening file: '" + pathToUtf8(file_path) + "'");
                    overall_success = false;
                }

                continue;
            }

            const std::string name_in_zip = zipEntryNameFromDiskPath(
                file_path, p_file_or_folder_path, save_hierarchy);

#if !defined(_WIN32)
            uint32_t const unix_zip_attrs =
                zip_unix_external_attributes(file_path.c_str());
#else
            uint32_t const unix_zip_attrs = 0;
#endif

            Timestamp time(file_path);
            if (!m_impl->add(input,
                             time.timestamp,
                             name_in_zip,
                             m_password,
                             p_flags,
                             unix_zip_attrs))
            {
                overall_success = false;
            }
        }
    }
    else // It's a single file
    {
        // Set progress for a single file
        m_impl->m_progress.total_files = 1;
        m_impl->m_progress.bytes_processed = 0;
        m_impl->m_progress.files_compressed = 0;

        std::ifstream input(p_file_or_folder_path, std::ios::binary);
        if (!input.is_open())
        {
            m_impl->m_progress.total_bytes =
                static_cast<uint64_t>(input.tellg());
            m_error_code =
                make_error_code(ZipperError::ADDING_ERROR,
                                "Failed opening file: '" +
                                    pathToUtf8(p_file_or_folder_path) + "'");
            return false;
        }

        const auto u8 = p_file_or_folder_path.filename().u8string();
        const std::string name_in_zip(u8.begin(), u8.end());

#if !defined(_WIN32)
        uint32_t const unix_zip_attrs =
            zip_unix_external_attributes(p_file_or_folder_path.c_str());
#else
        uint32_t const unix_zip_attrs = 0;
#endif

        Timestamp time(p_file_or_folder_path);
        overall_success = m_impl->add(input,
                                      time.timestamp,
                                      name_in_zip,
                                      m_password,
                                      p_flags,
                                      unix_zip_attrs);
    }

    m_impl->m_progress.status =
        overall_success ? Progress::Status::OK : Progress::Status::KO;

    if (overall_success)
    {
        overall_success = m_impl->maybeAutoFlush();
    }

    return overall_success;
}

// -------------------------------------------------------------------------
bool Zipper::open(const std::string& p_zip_name,
                  const std::string& p_password,
                  Zipper::OpenFlags p_open_flags)
{
    return open(utf8ToPath(p_zip_name), p_password, p_open_flags);
}

// -------------------------------------------------------------------------
bool Zipper::open(const std::filesystem::path& p_zip_name,
                  const std::string& p_password,
                  Zipper::OpenFlags p_open_flags)
{
    m_zip_name = p_zip_name;
    m_password = p_password;
    m_open_flags = p_open_flags;
    m_output_stream = nullptr;
    m_output_vector = nullptr;

    m_impl = std::make_unique<Impl>(*this, m_error_code);
    return reopen();
}

// -------------------------------------------------------------------------
bool Zipper::open(const std::string& p_zip_name, Zipper::OpenFlags p_open_flags)
{
    return open(p_zip_name, std::string(), p_open_flags);
}

// -------------------------------------------------------------------------
bool Zipper::open(std::iostream& p_buffer,
                  const std::string& p_password,
                  Zipper::OpenFlags p_open_flags)
{
    m_zip_name.clear();
    m_password = p_password;
    m_open_flags = p_open_flags;
    m_output_stream = &p_buffer;
    m_output_vector = nullptr;

    m_impl = std::make_unique<Impl>(*this, m_error_code);
    return reopen();
}

// -------------------------------------------------------------------------
bool Zipper::open(std::iostream& p_buffer, const std::string& p_password)
{
    m_zip_name.clear();
    m_password = p_password;
    m_open_flags = Zipper::OpenFlags::Overwrite;
    m_output_stream = &p_buffer;
    m_output_vector = nullptr;

    m_impl = std::make_unique<Impl>(*this, m_error_code);
    return reopen();
}

// -------------------------------------------------------------------------
bool Zipper::open(std::vector<unsigned char>& p_buffer,
                  const std::string& p_password)
{
    m_zip_name.clear();
    m_password = p_password;
    m_output_stream = nullptr;
    m_output_vector = &p_buffer;

    m_impl = std::make_unique<Impl>(*this, m_error_code);
    return reopen();
}

// -------------------------------------------------------------------------
bool Zipper::reopen()
{
    // Ensure impl is created if it wasn't (e.g., after default constructor
    // if added) For now, assuming constructor always creates impl or
    // throws.
    if (!m_impl)
    {
        m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                       "Zipper is not initialized");
        return false;
    }

    // If already open, close it first before reopening
    if (m_open)
    {
        close();
    }

    // Clear previous error before attempting open
    m_error_code = {};
    bool success = false;

    // Re-initialize based on the original construction mode
    if (!m_zip_name.empty())
    {
        success = m_impl->initFile(m_zip_name, m_open_flags);
    }
    else if (m_output_vector != nullptr)
    {
        success = m_impl->initWithVector(*m_output_vector);
    }
    else if (m_output_stream != nullptr)
    {
        success = m_impl->initWithStream(*m_output_stream);
    }
    else
    {
        // Should not happen if constructor logic is correct
        m_error_code =
            make_error_code(ZipperError::INTERNAL_ERROR,
                            "Invalid internal state for opening zip file");
        return false;
    }

    if (success)
    {
        m_open = true;
        m_error_code = {};
    }

    return success;
}

// -----------------------------------------------------------------------------
bool Zipper::checkValid()
{
    if (!m_impl)
    {
        m_error_code = make_error_code(ZipperError::INTERNAL_ERROR,
                                       "Zipper is not initialized");
        return false;
    }

    if (!m_open)
    {
        m_error_code = make_error_code(ZipperError::OPENING_ERROR,
                                       "Zip archive is not opened");
        return false;
    }

    return true;
}

} // namespace zipper
