//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#ifndef ZIPPER_UNZIPPER_HPP
#  define ZIPPER_UNZIPPER_HPP

#include <vector>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <memory>
#include <map>
#include <system_error>

namespace zipper {

class ZipEntry;

// *****************************************************************************
//! \brief Zip archive extractor/decompressor.
// *****************************************************************************
class Unzipper
{
public:

    // -------------------------------------------------------------------------
    //! \brief Regular zip decompressor (from zip archive file).
    //!
    //! \param[in] zipname: the path of the zip file.
    //! \param[in] password: the password used by the Zipper class (set empty
    //!   if no password is needed).
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    Unzipper(std::string const& zipname,
             std::string const& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief In-memory zip decompressor (from std::iostream).
    //!
    //! \param[in,out] buffer: the stream in which zipped entries are stored.
    //! \param[in] password: the password used by the Zipper class (set empty
    //!   if no password is needed).
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    Unzipper(std::istream& buffer,
             std::string const& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief In-memory zip decompressor (from std::vector).
    //!
    //! \param[in,out] buffer: the vector in which zipped entries are stored.
    //! \param[in] password: the password used by the Zipper class (set empty
    //!   if no password is needed).
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    Unzipper(std::vector<unsigned char>& buffer,
             std::string const& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief Call release() and close().
    // -------------------------------------------------------------------------
    ~Unzipper();

    // -------------------------------------------------------------------------
    //! \brief Return entries of the zip archive.
    // -------------------------------------------------------------------------
    std::vector<ZipEntry> entries();

    // -------------------------------------------------------------------------
    //! \brief Extract the whole zip archive using alternative destination names
    //! for existing files on the disk.
    //!
    //! \param[in] destination: the full path of the file to be created that
    //!   will hold uncompressed data. If no destination is given extract in the
    //!   same folder than the zip file.
    //! \param[in] alternativeNames: dictionary of alternative names for
    //!   existing files on disk (dictionary key: zip entry name, dictionary
    //!   data: newly desired path name on the disk).
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false. Call error() for more info.
    // -------------------------------------------------------------------------
    inline bool extractAll(const char* destination,
                           const std::map<std::string, std::string>& alternativeNames,
                           bool const replace = false)
    {
        return extractAll(std::string(destination), alternativeNames, replace);
    }

    bool extractAll(std::string const& destination,
                    const std::map<std::string, std::string>& alternativeNames,
                    bool const replace = false);

    // -------------------------------------------------------------------------
    //! \brief Extract the whole archive to the desired disk destination. If no
    //!   destination is given extract in the same folder than the zip file.
    //!
    //! \param[in] destination: the full path on the disk of the file to be
    //!   created that will hold uncompressed data. If no destination is given
    //!   extract in the same folder than the zip file.
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractAll(std::string const& destination, bool const replace = false);
    inline bool extractAll(const char* destination, bool const replace = false)
    {
        return extractAll(std::string(destination), replace);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract the whole archive to the same folder than the zip file.
    //!
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractAll(bool const replace = false);

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from the archive.
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[in] destination: the full path on the disk of the file to be
    //!   created that will hold uncompressed data. If no destination is given
    //!   extract in the same folder than the zip file.
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractEntry(std::string const& name,
                      std::string const& destination,
                      bool const replace = false);
    inline bool extractEntry(const char* name, const char* destination,
                             bool const replace = false)
    {
        return extractEntry(std::string(name), std::string(destination), replace);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from the archive in the same folder than
    //! the zip file.
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractEntry(std::string const& name, bool const replace = false);
    inline bool extractEntry(const char* name, bool const replace = false)
    {
        return extractEntry(std::string(name), replace);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from zip to memory (stream).
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[out] stream: the stream that will hold the extracted entry.
    //! \return true on success, else return false. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractEntryToStream(std::string const& name, std::ostream& stream);
    inline bool extractEntryToStream(const char* name, std::ostream& stream)
    {
        return extractEntryToStream(std::string(name), stream);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from zip to memory (vector).
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[out] vec: the vector that will hold the extracted entry.
    //! \return true on success, else return false. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractEntryToMemory(std::string const& name,
                              std::vector<unsigned char>& vec);

    inline bool extractEntryToMemory(const char* name, std::vector<unsigned char>& vec)
    {
        return extractEntryToMemory(std::string(name), vec);
    }

    // -------------------------------------------------------------------------
    //! \brief Relese memory. Called by the destructor.
    // -------------------------------------------------------------------------
    void close();

    // -------------------------------------------------------------------------
    //! \brief Get the error information when a method returned false.
    // -------------------------------------------------------------------------
    std::error_code const& error() const;

private:

    //! \brief Relese memory
    void release();

private:

    std::istream& m_ibuffer;
    std::vector<unsigned char>& m_vecbuffer;
    std::string m_zipname;
    std::string m_password;
    bool m_usingMemoryVector;
    bool m_usingStream;
    bool m_open;
    std::error_code m_error_code;

    struct Impl;
    Impl* m_impl;
};

// *************************************************************************
//! \brief
// *************************************************************************
class ZipEntry
{
public:

    ZipEntry() = default;
    ZipEntry(std::string const& name_,
             uint64_t compressed_size,
             uint64_t uncompressed_size,
             uint32_t year,
             uint32_t month,
             uint32_t day,
             uint32_t hour,
             uint32_t minute,
             uint32_t second,
             uint32_t dosdate_)
        : name(name_), compressedSize(compressed_size),
          uncompressedSize(uncompressed_size), dosdate(dosdate_)
    {
        // timestamp YYYY-MM-DD HH:MM:SS
        std::stringstream str;
        str << year << "-" << month << "-" << day << " "
            << hour << ":" << minute << ":" << second;
        timestamp = str.str();

        unixdate.tm_year = year;
        unixdate.tm_mon = month;
        unixdate.tm_mday = day;
        unixdate.tm_hour = hour;
        unixdate.tm_min = minute;
        unixdate.tm_sec = second;
    }

    ZipEntry(ZipEntry const& other)
        : ZipEntry(other.name,
                   other.compressedSize,
                   other.uncompressedSize,
                   other.unixdate.tm_year,
                   other.unixdate.tm_mon,
                   other.unixdate.tm_mday,
                   other.unixdate.tm_hour,
                   other.unixdate.tm_min,
                   other.unixdate.tm_sec,
                   other.dosdate)
    {}

    ZipEntry& operator=(ZipEntry const& other)
    {
        this->~ZipEntry(); // destroy
        new (this) ZipEntry(other); // copy construct in place
        return *this;
    }

    ZipEntry& operator=(ZipEntry && other)
    {
        this->~ZipEntry(); // destroy
        new (this) ZipEntry(other); // copy construct in place
        return *this;
    }

    inline bool valid() const { return !name.empty(); }

public:

    typedef struct
    {
        uint32_t tm_sec;
        uint32_t tm_min;
        uint32_t tm_hour;
        uint32_t tm_mday;
        uint32_t tm_mon;
        uint32_t tm_year;
    } tm_s;

    std::string name, timestamp;
    uint64_t compressedSize;
    uint64_t uncompressedSize;
    uint32_t dosdate;
    tm_s unixdate;
};

} // namespace zipper

#endif // ZIPPER_UNZIPPER_HPP
