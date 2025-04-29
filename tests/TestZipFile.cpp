#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#pragma GCC diagnostic pop

#define protected public
#define private public
#include "Zipper/Unzipper.hpp"
#include "Zipper/Zipper.hpp"
#undef protected
#undef private

#include "utils/Path.hpp"
#include <chrono>
#include <fstream>

using namespace zipper;

//=============================================================================
// Helper functions for tests
//=============================================================================
namespace helper {

/**
 * @brief Reads and returns the content of a file.
 * @param[in] p_file Path to the file to read.
 * @return The content of the file as a string.
 */
static std::string readFileContent(const std::string& p_file)
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
static bool checkFileExists(const std::string& p_file,
                            const std::string& p_content)
{
    return Path::exist(p_file) && Path::isFile(p_file) &&
           readFileContent(p_file) == p_content;
}

/**
 * @brief Checks if a file does not exist.
 * @param[in] p_file Path to the file to check.
 * @return true if the file does not exist, false otherwise.
 */
static bool checkFileDoesNotExist(const std::string& p_file)
{
    return !Path::exist(p_file) || !Path::isFile(p_file);
}

/**
 * @brief Checks if a directory exists.
 * @param[in] p_dir Path to the directory to check.
 * @return true if the directory exists, false otherwise.
 */
static bool checkDirExists(const std::string& p_dir)
{
    return Path::exist(p_dir) && Path::isDir(p_dir);
}

/**
 * @brief Creates a file with content.
 * @param[in] p_file Path to the file to create.
 * @param[in] p_content Content to write in the file.
 * @return true if successful, false otherwise.
 */
static bool createFile(const std::string& p_file, const std::string& p_content)
{
    Path::remove(p_file);

    std::ofstream ofs(p_file);
    ofs << p_content;
    ofs.flush();
    ofs.close();

    return checkFileExists(p_file, p_content);
}

/**
 * @brief Creates a directory.
 * @param[in] p_dir Path to the directory to create.
 * @return true if successful, false otherwise.
 */
static bool createDir(const std::string& p_dir)
{
    return Path::createDir(p_dir) && checkDirExists(p_dir);
}

/**
 * @brief Creates a file with content and adds it to the zipper.
 * @param[in] p_zipper The zipper instance.
 * @param[in] p_file_path Path of the temporary file to create.
 * @param[in] p_content Content to write in the file.
 * @param[in] p_entry_path Path of the entry in the zip archive.
 * @return true if successful, false otherwise.
 */
static bool zipAddFile(Zipper& p_zipper,
                       const std::string& p_file_path,
                       const std::string& p_content,
                       const std::string& p_entry_path)
{
    if (!helper::createFile(p_file_path, p_content))
    {
        return false;
    }

    std::ifstream ifs(p_file_path);
    bool res = p_zipper.add(ifs, p_entry_path, Zipper::SaveHierarchy);
    ifs.close();

    Path::remove(p_file_path);

    return res;
}
} // namespace helper

//=============================================================================
// Test Suite for basic opening and closing of zip files.
//=============================================================================
TEST(ZipperFileOps, NominalOpenings)
{
    const std::string zip_filename = "ziptest_nominal.zip";
    const std::string file1 = "test1_nominal.txt";
    const std::string file2 = "test2_nominal.txt";
    const std::string file3 = "test3_nominal.txt";
    const std::string content1 = "content nominal 1";
    const std::string content2 = "content nominal 2";
    const std::string content3 = "content nominal 3";

    // Clean up.
    Path::remove(zip_filename);
    ASSERT_FALSE(Path::exist(zip_filename));

    // Constructor with Overwrite flag.
    {
        Zipper zipper(zip_filename); // Default is Overwrite
        ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Verify content. Check if file1 is in the zip archive.
    {
        Unzipper unzipper(zip_filename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        unzipper.close();
        ASSERT_EQ(entries.size(), 1u);
        ASSERT_EQ(entries[0].name, file1);
    }

    // Reopen with Append
    {
        Zipper zipper(zip_filename, Zipper::OpenFlags::Append);
        ASSERT_TRUE(helper::zipAddFile(zipper, file3, content3, file3));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Verify content (file1, file3)
    {
        Unzipper unzipper(zip_filename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        unzipper.close();
        ASSERT_EQ(entries.size(), 2u);
        ASSERT_EQ(entries[0].name, file1);
        ASSERT_EQ(entries[1].name, file3);
    }

    // Constructor with Overwrite flag (second times)
    {
        Zipper zipper(zip_filename, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(helper::zipAddFile(zipper, file2, content2, file2));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Verify content. Check if file2 is in the zip archive and file1 is not.
    {
        Unzipper unzipper(zip_filename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        unzipper.close();
        ASSERT_EQ(entries.size(), 1u);
        ASSERT_EQ(entries[0].name, file2);
    }

    // Reopen with Append flag.
    {
        Zipper zipper(zip_filename, Zipper::OpenFlags::Overwrite);
        // FIXME zipper.open(Zipper::OpenFlags::Append);
        std::cout << "zipper.error() = " << zipper.error().message()
                  << std::endl;
        ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Verify content. Check if file1 is in the zip archive.
    {
        Unzipper unzipper(zip_filename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        unzipper.close();
        ASSERT_EQ(entries.size(), 1u);
        ASSERT_EQ(entries[0].name, file1);
    }

    // Reopen with Append
    {
        Zipper zipper(zip_filename, Zipper::OpenFlags::Append);
        ASSERT_TRUE(helper::zipAddFile(zipper, file3, content3, file3));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Verify content (file1, file3)
    {
        Unzipper unzipper(zip_filename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        unzipper.close();
        ASSERT_EQ(entries.size(), 2u);
        ASSERT_EQ(entries[0].name, file1);
        ASSERT_EQ(entries[1].name, file3);
    }

    // Reopen with Overwrite flag
    {
        Zipper zipper(zip_filename, Zipper::OpenFlags::Append);
        ASSERT_TRUE(zipper.open(Zipper::OpenFlags::Overwrite));
        ASSERT_TRUE(helper::zipAddFile(zipper, file2, content2, file2));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }
    // Verify content (only file2)
    {
        Unzipper unzipper(zip_filename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        unzipper.close();
        ASSERT_EQ(entries.size(), 1u);
        ASSERT_EQ(entries[0].name, file2);
    }

    // Final Clean up
    Path::remove(zip_filename);
}

//=============================================================================
// Test Suite for Zipper File Operations
//=============================================================================
TEST(ZipperFileOps, TryOpeningFolderInsteadOfZipFile)
{
    const std::vector<std::string> folder_paths = {
        "ziptest_folder", "ziptest_folder.zip", "ziptest_folder.zip.txt"};
    for (const auto& folder_path : folder_paths)
    {
        std::cout << "Testing folder: " << folder_path << std::endl;
        ASSERT_TRUE(helper::createDir(folder_path));

        // Try opening the folder as a zip file.
        try
        {
            Zipper zipper(folder_path);
            FAIL() << "Expected std::runtime_error for opening a directory";
        }
        catch (const std::runtime_error& e)
        {
            ASSERT_THAT(e.what(), testing::HasSubstr("Is a directory"));
        }

        // Try opening the folder as an unzipper.
        try
        {
            Unzipper unzipper(folder_path);
            FAIL() << "Expected std::runtime_error for opening a directory";
        }
        catch (const std::runtime_error& e)
        {
            ASSERT_THAT(e.what(), testing::HasSubstr("Is a directory"));
        }

        // Clean up.
        Path::remove(folder_path);
    }
}

//=============================================================================
// Test Suite with fake zip files
//=============================================================================
TEST(ZipperFileOps, TryOpeningFakeZipFiles)
{
    const std::vector<std::string> fake_zip_filenames = {
        "/home/qq/MyGitHub/zipper/tests/issues/foobar.txt"};

    for (const auto& fake_zip_filename : fake_zip_filenames)
    {
        // Create a fake zip file.
        ASSERT_TRUE(helper::createFile(fake_zip_filename, "foobar"));

        // Try opening the zip file with the Overwrite flag.
        try
        {
            std::cout << "Testing Zipper with " << fake_zip_filename
                      << std::endl;
            Zipper zipper(fake_zip_filename, Zipper::OpenFlags::Append);
            FAIL() << "Expected std::runtime_error for opening a bad zip file";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_THAT(e.what(), testing::HasSubstr("Not a zip file"));
        }

        // Try opening the unzipper.
        try
        {
            std::cout << "Testing Unzipper with " << fake_zip_filename
                      << std::endl;
            Unzipper unzipper(fake_zip_filename);
            FAIL() << "Expected std::runtime_error for opening a bad zip file";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_THAT(e.what(), testing::HasSubstr("Not a zip file"));
        }

        // Clean up.
        Path::remove(fake_zip_filename);
    }
}

//=============================================================================
// Test Suite with bad zip files
//=============================================================================
TEST(ZipperFileOps, TryOpeningBadZipFiles)
{
    const std::string zip_poc0 =
        "/home/qq/MyGitHub/zipper/tests/issues/poc0.zip";
    const std::string zip_poc1 =
        "/home/qq/MyGitHub/zipper/tests/issues/poc1.zip";

    // Try opening the zip file with the Append flag.
    try
    {
        std::cout << "Testing Zipper with " << zip_poc0 << std::endl;
        Zipper zipper(zip_poc0, Zipper::OpenFlags::Append);
    }
    catch (const std::runtime_error& e)
    {
        FAIL() << "Did not expect std::runtime_error";
    }

    // Try unzipping the bad zip file.
    try
    {
        std::cout << "Testing Unzipper with " << zip_poc0 << std::endl;
        Unzipper unzipper(zip_poc0);
    }
    catch (const std::runtime_error& e)
    {
        FAIL() << "Did not expect std::runtime_error";
    }

    // Try opening the zip file with the Append flag.
    try
    {
        std::cout << "Testing Zipper with " << zip_poc1 << std::endl;
        Zipper zipper(zip_poc1, Zipper::OpenFlags::Append);
        FAIL() << "Expected std::runtime_error for bad zip file";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("Bad file descriptor"));
    }

    // Try unzipping the bad zip file.
    try
    {
        std::cout << "Testing Unzipper with " << zip_poc1 << std::endl;
        Unzipper unzipper(zip_poc1);
        unzipper.extractAll("eee/");
        unzipper.close();
        FAIL() << "Expected std::runtime_error for bad zip file";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("Bad file descriptor"));
    }
}

//=============================================================================
// Test Suite with insufficient permissions
//=============================================================================
#if !defined(_WIN32) // Permission tests are more reliable on Unix-like
TEST(ZipperFileOps, TryOpeningWithInsufficientPermissions)
{
    const std::string protectedPath =
        "/root/no_permission_test.zip"; // Path likely requiring root

    try
    {
        Zipper zipper(protectedPath);
        FAIL() << "Expected std::runtime_error for permission denied";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("Permission denied"));
    }

    try
    {
        Unzipper unzipper(protectedPath);
        FAIL() << "Expected std::runtime_error for permission denied";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("Permission denied"));
    }
}
#endif

//=============================================================================
// Non-existent file
//=============================================================================
TEST(ZipperFileOps, TryOpeningNonExistentFile)
{
    const std::string nonExistentFile = "non_existent_file.zip";

    // Try opening the non-existent file with the Overwrite flag.
    try
    {
        Path::remove(nonExistentFile);
        Zipper zipper(nonExistentFile, Zipper::OpenFlags::Append);
        FAIL() << "Expected std::runtime_error for non-existent file";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("No such file or directory"));
    }

    // Try opening the non-existent file with the Append flag.
    try
    {
        Path::remove(nonExistentFile);
        Zipper zipper(nonExistentFile);
        ASSERT_FALSE(helper::checkFileDoesNotExist(nonExistentFile));
    }
    catch (const std::runtime_error& e)
    {
        FAIL() << "Did not expect std::runtime_error";
    }

    // Try opening the non-existent file.
    try
    {
        Path::remove(nonExistentFile);
        Unzipper unzipper(nonExistentFile);
        FAIL() << "Expected std::runtime_error for non-existent file";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("No such file or directory"));
    }

    // Clean up.
    Path::remove(nonExistentFile);
}