#ifndef UNIT_TESTS_HELPER_HPP
#define UNIT_TESTS_HELPER_HPP

#include "utils/Path.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

//=============================================================================
// Helper functions for tests
//=============================================================================
namespace helper {

/**
 * @brief Reads and returns the content of a file.
 * @param[in] p_file Path to the file to read.
 * @return The content of the file as a string.
 */
inline std::string readFileContent(const std::string& p_file)
{
    std::ifstream ifs(p_file);
    std::string str((std::istreambuf_iterator<char>(ifs)),
                    std::istreambuf_iterator<char>());
    return str.c_str();
}

/**
 * @brief Checks if a file exists and has the expected content.
 * @param[in] p_file Path to the file to check.
 * @param[in] p_content Content to check in the file.
 * @return true if the file exists and has the expected content, false
 * otherwise.
 */
inline bool checkFileExists(const std::string& p_file,
                            const std::string& p_content)
{
    return zipper::Path::exist(p_file) && zipper::Path::isFile(p_file) &&
           readFileContent(p_file) == p_content;
}

inline bool checkFileExists(const std::string& p_file)
{
    return zipper::Path::exist(p_file) && zipper::Path::isFile(p_file);
}

/**
 * @brief Checks if a file does not exist.
 * @param[in] p_file Path to the file to check.
 * @return true if the file does not exist, false otherwise.
 */
inline bool checkFileDoesNotExist(const std::string& p_file)
{
    return !zipper::Path::exist(p_file) || !zipper::Path::isFile(p_file);
}

/**
 * @brief Checks if a directory exists.
 * @param[in] p_dir Path to the directory to check.
 * @return true if the directory exists, false otherwise.
 */
inline bool checkDirExists(const std::string& p_dir)
{
    return zipper::Path::exist(p_dir) && zipper::Path::isDir(p_dir);
}

/**
 * @brief Checks if a directory does not exist.
 * @param[in] p_dir Path to the directory to check.
 * @return true if the directory does not exist, false otherwise.
 */
inline bool checkDirDoesNotExist(const std::string& p_dir)
{
    return !zipper::Path::exist(p_dir) && !zipper::Path::isDir(p_dir);
}

/**
 * @brief Creates a file with content.
 * @param[in] p_file Path to the file to create.
 * @param[in] p_content Content to write in the file.
 * @return true if successful, false otherwise.
 */
inline bool createFile(const std::string& p_file, const std::string& p_content)
{
    zipper::Path::remove(p_file);

    std::ofstream ofs(p_file);
    ofs << p_content;
    ofs.flush();
    ofs.close();

    return checkFileExists(p_file, p_content);
}

/**
 * @brief Removes a file or directory.
 * @param[in] p_file Path to the file or directory to remove.
 * @return true if successful, false otherwise.
 */
inline bool removeFileOrDir(const std::string& p_file)
{
    if (zipper::Path::isFile(p_file))
    {
        zipper::Path::remove(p_file);
        return checkFileDoesNotExist(p_file);
    }
    else if (zipper::Path::isDir(p_file))
    {
        return zipper::Path::remove(p_file);
        return (!checkDirExists(p_file));
    }
    return true;
}

/**
 * @brief Creates a directory.
 * @param[in] p_dir Path to the directory to create.
 * @return true if successful, false otherwise.
 */
inline bool createDir(const std::string& p_dir)
{
    return removeFileOrDir(p_dir) && zipper::Path::createDir(p_dir) &&
           checkDirExists(p_dir);
}

/**
 * @brief Checks if a directory is empty.
 * @param[in] p_dir Path to the directory to check.
 * @return true if the directory is empty, false otherwise.
 */
inline bool isDirEmpty(const std::string& p_dir)
{
    if (!zipper::Path::isDir(p_dir))
        return false;

    std::vector<std::string> entries = zipper::Path::filesFromDir(p_dir, false);
    return entries.empty();
}

/**
 * @brief Creates a file with content and adds it to the zipper.
 * @param[in] p_zipper The zipper instance.
 * @param[in] p_file_path Path of the temporary file to create.
 * @param[in] p_content Content to write in the file.
 * @param[in] p_entry_path Path of the entry in the zip archive.
 * @return true if successful, false otherwise.
 */
inline bool zipAddFile(zipper::Zipper& p_zipper,
                       const char* p_file_path, // FIXME useless: to be removed
                       const char* p_content,
                       const char* p_entry_path)
{
    if (!helper::createFile(p_file_path, p_content))
    {
        return false;
    }

    std::ifstream ifs(p_file_path);
    bool res = p_zipper.add(ifs, p_entry_path, zipper::Zipper::SaveHierarchy);
    ifs.close();

    helper::removeFileOrDir(p_file_path);

    return res;
}

//-----------------------------------------------------------------------------
//! \brief Convert an integer to a base-36 string (digits 0-9, letters A-Z).
//! \param[in] value The integer value to convert (must be >= 0).
//! \return The base-36 string representation.
//-----------------------------------------------------------------------------
inline std::string intToBase36(size_t value)
{
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (value < 36)
        return std::string(1, digits[value]);

    // On commence à 2 digits après 35 ("00"), puis 3, etc.
    size_t n = value - 36;
    size_t width = 2;
    size_t max = 36 * 36;
    while (n >= max)
    {
        n -= max;
        ++width;
        max *= 36;
    }

    std::string result(width, '0');
    size_t i = width;
    while (i-- > 0)
    {
        result[i] = digits[n % 36];
        n /= 36;
    }
    return result;
}

} // namespace helper

#endif // UNIT_TESTS_HELPER_HPP