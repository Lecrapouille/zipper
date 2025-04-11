//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#ifndef ZIPPER_ZIPPER_HPP
#  define ZIPPER_ZIPPER_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <system_error>

namespace zipper {

// *************************************************************************
//! \brief Zip archive compressor.
// *************************************************************************
class Zipper
{
public:

    // -------------------------------------------------------------------------
    //! \brief Archive opening flags.
    // -------------------------------------------------------------------------
    enum openFlags
    {
        //! \brief Overwrite existing zip file
        Overwrite,
        //! \brief Append to existing zip file
        Append
    };

    // -------------------------------------------------------------------------
    //! \brief Compression options for files.
    // -------------------------------------------------------------------------
    enum zipFlags
    {
        //! \brief Minizip option: -0 Store only (no compression).
        Store = 0x00,
        //! \brief Minizip option: -1 Compress faster (less compression).
        Faster = 0x01,
        //! \brief Minizip option: -5 Medium compression.
        Medium = 0x05,
        //! \brief Minizip option: -9 Better compression (slower).
        Better = 0x09,
        //! \brief Preserve directory hierarchy when adding files.
        SaveHierarchy = 0x40
    };

    // -------------------------------------------------------------------------
    //! \brief Regular zip compression (inside a disk zip archive file) with a
    //! password.
    //!
    //! \param[in] zipname Path where to create the zip file.   
    //! \param[in] password Optional password (empty for no password protection).
    //! \param[in] flags Overwrite (default) or append to existing zip file.
    //! \throw std::runtime_error if an error occurs during initialization.
    // -------------------------------------------------------------------------
    Zipper(const std::string& zipname, const std::string& password,
           Zipper::openFlags flags = Zipper::openFlags::Overwrite);

    // -------------------------------------------------------------------------
    //! \brief Regular zip compression (inside a disk zip archive file) without
    //! password.
    //!
    //! \param[in] zipname Path where to create the zip file.
    //! \param[in] flags Overwrite (default) or append to existing zip file.
    //! \throw std::runtime_error if an error occurs during initialization.
    // -------------------------------------------------------------------------
    Zipper(const std::string& zipname, Zipper::openFlags flags = Zipper::openFlags::Overwrite)
        : Zipper(zipname, std::string(), flags)
    {}

    // -------------------------------------------------------------------------
    //! \brief In-memory zip compression (storage inside std::iostream).
    //!
    //! \param[in,out] buffer Stream in which to store zipped files.
    //! \param[in] password Optional password (empty for no password protection).
    //! \throw std::runtime_error if an error occurs during initialization.
    // -------------------------------------------------------------------------
    Zipper(std::iostream& buffer, const std::string& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief In-memory zip compression (storage inside std::vector).
    //!
    //! \param[in,out] buffer Vector in which to store zipped files.
    //! \param[in] password Optional password (empty for no password protection).
    //! \throw std::runtime_error if an error occurs during initialization.
    // -------------------------------------------------------------------------
    Zipper(std::vector<unsigned char>& buffer,
           const std::string& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief Calls close() method.
    // -------------------------------------------------------------------------
    ~Zipper();

    // -------------------------------------------------------------------------
    //! \brief Compress data from source with a given timestamp in the archive
    //! with the given name.
    //!
    //! \param[in,out] source Data stream to compress.
    //! \param[in] timestamp Desired timestamp for the file.
    //! \param[in] nameInZip Desired name for the file inside the archive.
    //! \param[in] flags Compression options (faster, better, etc.).
    //! \return true on success, false on failure.
    //! \throw std::runtime_error if an error occurs during compression.
    // -------------------------------------------------------------------------
    bool add(std::istream& source, const std::tm& timestamp, const std::string& nameInZip,
             Zipper::zipFlags flags = Zipper::zipFlags::Better);

    // -------------------------------------------------------------------------
    //! \brief Compress data from source in the archive with the given name.
    //! No timestamp will be stored.
    //!
    //! \param[in,out] source Data stream to compress.
    //! \param[in] nameInZip Desired name for the file inside the archive.
    //! \param[in] flags Compression options (faster, better, etc.).
    //! \return true on success, false on failure.
    //! \throw std::runtime_error if an error occurs during compression.
    // -------------------------------------------------------------------------
    bool add(std::istream& source, const std::string& nameInZip,
             Zipper::zipFlags flags = Zipper::zipFlags::Better);

    bool add(std::istream& source,
             Zipper::zipFlags flags = Zipper::zipFlags::Better)
    {
        return add(source, std::string(), flags);
    }

    // -------------------------------------------------------------------------
    //! \brief Compress a folder or a file in the archive.
    //!
    //! \param[in] fileOrFolderPath Path to the file or folder to compress.
    //! \param[in] flags Compression options (faster, better, etc.).
    //! \return true on success, false on failure.
    //! \throw std::runtime_error if an error occurs during compression.
    // -------------------------------------------------------------------------
    bool add(const std::string& fileOrFolderPath,
             Zipper::zipFlags flags = Zipper::zipFlags::Better);


    bool add(const char* fileOrFolderPath,
             Zipper::zipFlags flags = Zipper::zipFlags::Better)
    {
        return add(std::string(fileOrFolderPath), flags);
    }

    // -------------------------------------------------------------------------
    //! \brief Closes the zip archive.
    //! 
    //! Depending on the constructor used, this method will close the access to
    //! the zip file, flush the stream, or release memory.
    //! \note This method is called by the destructor.
    // -------------------------------------------------------------------------
    void close();

    // -------------------------------------------------------------------------
    //! \brief Opens or reopens the zip archive.
    //!
    //! To be called after a close(). Depending on the constructor used, this
    //! method will open the zip file or reserve buffers.
    //! \param[in] flags Overwrite or append (default) to existing zip file
    //! \return true on success, false on failure
    //! \note This method is not called by the constructor.
    // -------------------------------------------------------------------------
    bool open(Zipper::openFlags flags = Zipper::openFlags::Append);

    // -------------------------------------------------------------------------
    //! \brief Get the error information when a method returned false.
    //! \return Reference to the error code.
    // -------------------------------------------------------------------------
    std::error_code const& error() const;

private:

    void release();

private:

    std::iostream& m_obuffer;
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

} // namespace zipper

#endif // ZIPPER_ZIPPER_HPP
