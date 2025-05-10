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
} // namespace helper

#endif // UNIT_TESTS_HELPER_HPP