//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#ifndef ZIPPER_UNZIPPER_HPP
#  define ZIPPER_UNZIPPER_HPP

#include <vector>
#include <string>
#include <map>
#include <system_error>
#include <iostream>
#include <sstream>

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
    //! \param[in] zipname Path of the zip file to extract.
    //! \param[in] password Optional password used during compression (empty if no password).
    //! \throw std::runtime_error if an error occurs during initialization.
    // -------------------------------------------------------------------------
    Unzipper(std::string const& zipname,
             std::string const& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief In-memory zip decompressor (from std::iostream).
    //!
    //! \param[in,out] buffer Stream containing zipped entries to extract.
    //! \param[in] password Optional password used during compression (empty if no password).
    //! \throw std::runtime_error if an error occurs during initialization.
    // -------------------------------------------------------------------------
    Unzipper(std::istream& buffer,
             std::string const& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief In-memory zip decompressor (from std::vector).
    //!
    //! \param[in,out] buffer Vector containing zipped entries to extract.
    //! \param[in] password Optional password used during compression (empty if no password).
    //! \throw std::runtime_error if an error occurs during initialization.
    // -------------------------------------------------------------------------
    Unzipper(std::vector<unsigned char>& buffer,
             std::string const& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief Calls release() and close() methods.
    // -------------------------------------------------------------------------
    ~Unzipper();

    // -------------------------------------------------------------------------
    //! \brief Returns all entries contained in the zip archive.
    //! \return Vector of ZipEntry objects.
    // -------------------------------------------------------------------------
    std::vector<ZipEntry> entries();

    // -------------------------------------------------------------------------
    //! \brief Extract the whole zip archive using alternative destination names
    //! for existing files on the disk.
    //!
    //! \param[in] destination Full path where files will be extracted (if empty, extracts
    //!            to same folder as the zip file).
    //! \param[in] alternativeNames Dictionary of alternative names for existing files
    //!            (key: zip entry name, value: desired path name on disk).
    //! \param[in] replace If false (default), throws an exception when a file already exists.
    //! \return true on success, false on failure. Call error() for more info.
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
    //! \brief Extract the whole archive to the desired disk destination.
    //!
    //! \param[in] destination Full path where files will be extracted (if empty, extracts
    //!            to same folder as the zip file).
    //! \param[in] replace If false (default), throws an exception when a file already exists.
    //! \return true on success, false on failure. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractAll(std::string const& destination, bool const replace = false);
    inline bool extractAll(const char* destination, bool const replace = false)
    {
        return extractAll(std::string(destination), replace);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract the whole archive to the same folder as the zip file.
    //!
    //! \param[in] replace If false (default), throws an exception when a file already exists.
    //! \return true on success, false on failure. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractAll(bool const replace = false);

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from the archive.
    //!
    //! \param[in] name Entry path inside the zip archive.
    //! \param[in] destination Full path where the file will be extracted.
    //! \param[in] replace If false (default), throws an exception when the file already exists.
    //! \return true on success, false on failure. Call error() for more info.
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
    //! \brief Extract a single entry to the same folder as the zip file.
    //!
    //! \param[in] name Entry path inside the zip archive.
    //! \param[in] replace If false (default), throws an exception when the file already exists.
    //! \return true on success, false on failure. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractEntry(std::string const& name, bool const replace = false);
    inline bool extractEntry(const char* name, bool const replace = false)
    {
        return extractEntry(std::string(name), replace);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from zip to memory (stream).
    //!
    //! \param[in] name Entry path inside the zip archive.
    //! \param[out] stream Stream that will receive the extracted entry data.
    //! \return true on success, false on failure. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractEntryToStream(std::string const& name, std::ostream& stream);
    inline bool extractEntryToStream(const char* name, std::ostream& stream)
    {
        return extractEntryToStream(std::string(name), stream);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from zip to memory (vector).
    //!
    //! \param[in] name Entry path inside the zip archive.
    //! \param[out] vec Vector that will receive the extracted entry data.
    //! \return true on success, false on failure. Call error() for more info.
    // -------------------------------------------------------------------------
    bool extractEntryToMemory(std::string const& name,
                              std::vector<unsigned char>& vec);

    inline bool extractEntryToMemory(const char* name, std::vector<unsigned char>& vec)
    {
        return extractEntryToMemory(std::string(name), vec);
    }

    // -------------------------------------------------------------------------
    //! \brief Closes the archive. Called by the destructor.
    // -------------------------------------------------------------------------
    void close();

    // -------------------------------------------------------------------------
    //! \brief Get the error information when a method returned false.
    //! \return Reference to the error code.
    // -------------------------------------------------------------------------
    std::error_code const& error() const;

private:

    //! \brief Releases allocated resources.
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
//! \brief Class representing an entry in a zip archive.
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

    //! \brief Checks if the entry has a valid name
    //! \return true if the entry name is not empty
    inline bool valid() const { return !name.empty(); }

public:

    //! \brief Structure representing a date and time
    typedef struct
    {
        uint32_t tm_sec;   //!< Seconds (0-59)
        uint32_t tm_min;   //!< Minutes (0-59)
        uint32_t tm_hour;  //!< Hours (0-23)
        uint32_t tm_mday;  //!< Day of month (1-31)
        uint32_t tm_mon;   //!< Month (1-12)
        uint32_t tm_year;  //!< Year (full year, e.g. 2022)
    } tm_s;

    std::string name;      //!< Name of the entry in the zip archive
    std::string timestamp; //!< Formatted timestamp string (YYYY-MM-DD HH:MM:SS)
    uint64_t compressedSize;   //!< Size of the compressed data in bytes
    uint64_t uncompressedSize; //!< Original size of the data in bytes
    uint32_t dosdate;      //!< DOS-format date
    tm_s unixdate;         //!< UNIX-format date and time
};

} // namespace zipper

#endif // ZIPPER_UNZIPPER_HPP
