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
 * @brief Removes a file or directory.
 * @param[in] p_file Path to the file or directory to remove.
 * @return true if successful, false otherwise.
 */
static bool removeFileOrDir(const std::string& p_file)
{
    if (Path::isFile(p_file))
    {
        Path::remove(p_file);
        return checkFileDoesNotExist(p_file);
    }
    else if (Path::isDir(p_file))
    {
        return Path::remove(p_file);
        return (!checkDirExists(p_file));
    }
    return true;
}

/**
 * @brief Creates a directory.
 * @param[in] p_dir Path to the directory to create.
 * @return true if successful, false otherwise.
 */
static bool createDir(const std::string& p_dir)
{
    return removeFileOrDir(p_dir) && Path::createDir(p_dir) &&
           checkDirExists(p_dir);
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
// Test Suite for opening and closing zip files.
//=============================================================================
TEST(ZipperFileOps, OpenAndClose)
{
    // Opening from a dummy constructor.
    Zipper zipper1;
    ASSERT_FALSE(zipper1.isOpen());
    zipper1.close();
    ASSERT_FALSE(zipper1.isOpen());
    zipper1.reopen();
    ASSERT_FALSE(zipper1.isOpen());

    // Opening from a non existing zip file.
    zipper1.open("ziptest_nominal.zip");
    ASSERT_TRUE(zipper1.isOpen());
    zipper1.close();
    ASSERT_FALSE(zipper1.isOpen());

    // Opening from an existing zip file.
    ASSERT_TRUE(Path::exist("ziptest_nominal.zip"));
    ASSERT_TRUE(Path::isFile("ziptest_nominal.zip"));
    Zipper zipper2("ziptest_nominal.zip");
    ASSERT_TRUE(zipper2.isOpen());
    zipper2.close();
    ASSERT_FALSE(zipper2.isOpen());
    zipper2.reopen();
    ASSERT_TRUE(zipper2.isOpen());
    zipper2.close();
    ASSERT_FALSE(zipper2.isOpen());

    // Opening an existent zipfile.
    ASSERT_TRUE(Path::exist("ziptest_nominal.zip"));
    ASSERT_TRUE(Path::isFile("ziptest_nominal.zip"));
    Unzipper unzipper2("ziptest_nominal.zip");
    ASSERT_TRUE(unzipper2.isOpen());
    unzipper2.close();
    ASSERT_FALSE(unzipper2.isOpen());
    // TODO: to implement
    // unzipper2.open();
    // ASSERT_TRUE(zipper2.isOpen());
    // unzipper2.close();

    // Opening from a non-existent zip file.
    helper::removeFileOrDir("ziptest_nominal.zip");
    try
    {
        Unzipper unzipper("ziptest_nominal.zip");
        FAIL() << "Expected exception for opening a non-existent zip file";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("No such file or directory"));
    }

    // Clean up.
    helper::removeFileOrDir("ziptest_nominal.zip");
}

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

    std::vector<std::string> passwords = {"", "1234567890"};

    for (const auto& password : passwords)
    {
        std::cout << "Testing with password: '" << password << "'" << std::endl;

        // Clean up.
        ASSERT_TRUE(helper::removeFileOrDir(zip_filename));

        // Constructor with Overwrite flag.
        {
            Zipper zipper(zip_filename, password); // Default is Overwrite
            ASSERT_TRUE(zipper.isOpen());
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }

        // Verify content. Check if file1 is in the zip archive.
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_TRUE(unzipper.isOpen());
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            auto entries = unzipper.entries();
            unzipper.close();
            ASSERT_EQ(entries.size(), 1u);
            ASSERT_EQ(entries[0].name, file1);
        }

        // Reopen with Append
        {
            Zipper zipper(zip_filename, password, Zipper::OpenFlags::Append);
            ASSERT_TRUE(zipper.isOpen());
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            ASSERT_TRUE(helper::zipAddFile(zipper, file3, content3, file3));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }

        // Verify content (file1, file3)
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            ASSERT_TRUE(unzipper.isOpen());
            auto entries = unzipper.entries();
            unzipper.close();
            ASSERT_EQ(entries.size(), 2u);
            ASSERT_EQ(entries[0].name, file1);
            ASSERT_EQ(entries[1].name, file3);
        }

        // Constructor with Overwrite flag (second times)
        {
            Zipper zipper(zip_filename, password, Zipper::OpenFlags::Overwrite);
            ASSERT_TRUE(helper::zipAddFile(zipper, file2, content2, file2));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }

        // Verify content. Check if file2 is in the zip archive and file1 is
        // not.
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            auto entries = unzipper.entries();
            unzipper.close();
            ASSERT_EQ(entries.size(), 1u);
            ASSERT_EQ(entries[0].name, file2);
        }

        // Reopen with Append flag.
        {
            Zipper zipper(zip_filename, password, Zipper::OpenFlags::Overwrite);
            ASSERT_TRUE(
                zipper.open(zip_filename, password, Zipper::OpenFlags::Append));
            ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }

        // Verify content. Check if file1 is in the zip archive.
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            auto entries = unzipper.entries();
            unzipper.close();
            ASSERT_EQ(entries.size(), 1u);
            ASSERT_EQ(entries[0].name, file1);
        }

        // Reopen with Append
        {
            Zipper zipper(zip_filename, password, Zipper::OpenFlags::Append);
            ASSERT_TRUE(helper::zipAddFile(zipper, file3, content3, file3));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }

        // Verify content (file1, file3)
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            auto entries = unzipper.entries();
            unzipper.close();
            ASSERT_EQ(entries.size(), 2u);
            ASSERT_EQ(entries[0].name, file1);
            ASSERT_EQ(entries[1].name, file3);
        }

        // Reopen with Overwrite flag
        {
            Zipper zipper(zip_filename, password, Zipper::OpenFlags::Append);
            ASSERT_TRUE(zipper.open(
                zip_filename, password, Zipper::OpenFlags::Overwrite));
            ASSERT_TRUE(helper::zipAddFile(zipper, file2, content2, file2));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }
        // Verify content (only file2)
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            auto entries = unzipper.entries();
            unzipper.close();
            ASSERT_EQ(entries.size(), 1u);
            ASSERT_EQ(entries[0].name, file2);
        }

        // Final Clean up
        ASSERT_TRUE(helper::removeFileOrDir(zip_filename));
    }
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
        ASSERT_TRUE(helper::removeFileOrDir(folder_path));
    }
}

//=============================================================================
// Test Suite with fake zip files
//=============================================================================
TEST(ZipperFileOps, TryOpeningFakeZipFiles)
{
    const std::vector<std::string> fake_zip_filenames = {"foobar.txt"};

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
        ASSERT_TRUE(helper::removeFileOrDir(fake_zip_filename));
    }
}

//=============================================================================
// Test Suite with bad zip files
//=============================================================================
TEST(ZipperFileOps, TryOpeningBadZipFiles)
{
    const std::string zip_poc0 = "issues/poc0.zip";
    const std::string zip_poc1 = "issues/poc1.zip";

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
        ASSERT_THAT(e.what(), testing::HasSubstr("Not a zip file"));
    }

    // Try unzipping the bad zip file.
    try
    {
        std::cout << "Testing Unzipper with " << zip_poc1 << std::endl;
        Unzipper unzipper(zip_poc1);
        FAIL() << "Expected std::runtime_error for bad zip file";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("Not a zip file"));
    }
}

//=============================================================================
// Test Suite with insufficient permissions
//=============================================================================
#if !defined(_WIN32) // Permission tests are more reliable on Unix-like
TEST(ZipperFileOps, TryOpeningWithInsufficientPermissions)
{
    const std::string protected_path =
#    if defined(__APPLE__)
        "/System/Library/no_permission_test.zip";
#    elif defined(__linux__)
        "/root/no_permission_test.zip";
#    else
#        error "Unsupported platform"
#    endif

    try
    {
        Zipper zipper(protected_path);
        FAIL() << "Expected std::runtime_error for permission denied";
    }
    catch (const std::runtime_error& e)
    {
#    if defined(__APPLE__)
        GTEST_SKIP() << "TODO MacOS: why No such file or directory ?";
#    endif
        ASSERT_THAT(e.what(), testing::HasSubstr("Permission denied"));
    }

    try
    {
        Unzipper unzipper(protected_path);
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
        ASSERT_TRUE(helper::removeFileOrDir(nonExistentFile));
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
        ASSERT_TRUE(helper::removeFileOrDir(nonExistentFile));
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
        ASSERT_TRUE(helper::removeFileOrDir(nonExistentFile));
        Unzipper unzipper(nonExistentFile);
        FAIL() << "Expected std::runtime_error for non-existent file";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("No such file or directory"));
    }

    // Clean up.
    ASSERT_TRUE(helper::removeFileOrDir(nonExistentFile));
}

//=============================================================================
// Dummy zip files, Dummy file, Dummy directory
//=============================================================================
TEST(ZipperFileOps, DummyStuffs)
{
    Zipper zipper1("issues/dummy.zip", Zipper::OpenFlags::Append);
    ASSERT_TRUE(zipper1.isOpen());
    zipper1.close();

    Unzipper unzipper1("issues/dummy.zip");
    ASSERT_TRUE(unzipper1.isOpen());
    unzipper1.close();

    Zipper zipper2("foo.zip");
    ASSERT_TRUE(zipper2.isOpen());
    ASSERT_TRUE(helper::zipAddFile(zipper2, "dummy.txt", "", "dummy.txt"));

    helper::createDir("dummy_dir");
    // TODO
    // ASSERT_TRUE(zipper2.addDir("dummy_dir"));
    zipper2.close();

    helper::removeFileOrDir("foo.zip");
    helper::removeFileOrDir("dummy_dir");
}

//=============================================================================
// Test Suite with Add Operations
// FIXME: missing dummy folder insertion
//=============================================================================
TEST(ZipperFileOps, AddOperations)
{
    const std::string zipFilename = "ziptest_add.zip";
    const std::string file1 = "file1_add.txt";
    const std::string content1 = "content add 1";
    const std::string folder1 = "folder1_add/";
    const std::string fileInFolder = folder1 + "file_in_folder.txt";
    const std::string fileNotInFolder =
        "file_in_folder.txt"; // Because of SaveHierarchy
    const std::string contentInFolder = "content in folder";
    const std::string emptyFolder = "empty_folder_add/";
    const std::string extractDir = "extract_dir/";

    std::vector<std::string> passwords = {"", "1234567890"};

    for (const auto& password : passwords)
    {
        std::cout << "Testing with password: '" << password << "'" << std::endl;

        // Setup directories and files
        ASSERT_TRUE(helper::removeFileOrDir(zipFilename));
        ASSERT_TRUE(helper::createDir(folder1));
        ASSERT_TRUE(helper::createDir(emptyFolder));
        ASSERT_TRUE(helper::createFile(fileInFolder, contentInFolder));
        ASSERT_TRUE(helper::createFile(file1, content1));
        ASSERT_TRUE(helper::removeFileOrDir(fileNotInFolder));

        // Test adding files to zip
        {
            std::cout << "Adding files to zip" << std::endl;
            Zipper zipper(zipFilename, password, Zipper::OpenFlags::Overwrite);

            // Add single file (no hierarchy)
            ASSERT_TRUE(
                zipper.add(file1)); // Flags default to Better, no hierarchy
            ASSERT_FALSE(zipper.error()) << zipper.error().message();

            // Add folder (with hierarchy)
            ASSERT_TRUE(zipper.add(
                folder1, Zipper::ZipFlags::Better | Zipper::SaveHierarchy));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();

            // Add folder (without hierarchy - should only add files at root)
            ASSERT_TRUE(zipper.add(
                folder1, Zipper::ZipFlags::Store)); // No SaveHierarchy flag
            ASSERT_FALSE(zipper.error()) << zipper.error().message();

            // Add empty folder (with hierarchy - might add a dir entry or
            // nothing)
            ASSERT_TRUE(zipper.add(
                emptyFolder, Zipper::ZipFlags::Faster | Zipper::SaveHierarchy));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();

            // Add non-existent file (should fail)
            ASSERT_FALSE(zipper.add("non_existent_file_add.txt"));
            ASSERT_THAT(zipper.error().message(),
                        testing::HasSubstr("Cannot open file"));

            zipper.close();
        }

        // Clean up
        std::cout << "Cleaning up" << std::endl;
        ASSERT_TRUE(helper::removeFileOrDir(folder1));
        ASSERT_TRUE(helper::removeFileOrDir(emptyFolder));
        ASSERT_TRUE(helper::removeFileOrDir(file1));
        ASSERT_TRUE(helper::removeFileOrDir(fileNotInFolder));

        // Verify zip content
        {
            std::cout << "Verifying zip content" << std::endl;

            Unzipper unzipper(zipFilename, password);
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            auto entries = unzipper.entries();
            unzipper.close();

            ASSERT_EQ(entries.size(), 3u);
            ASSERT_STREQ(entries[0].name.c_str(), "file1_add.txt");
            ASSERT_STREQ(entries[1].name.c_str(),
                         "folder1_add/file_in_folder.txt");
            ASSERT_STREQ(entries[2].name.c_str(), "file_in_folder.txt");
        }

        // Extract single entry to current dir
        {
            std::cout << "Extracting entries to current dir:" << std::endl;

            Unzipper unzipper(zipFilename, password);
            auto entries = unzipper.entries();

            for (size_t i = 0; i < entries.size(); ++i)
            {
                const auto& entry = entries[i];
                std::cout << "  - Extracting " << entry.name << std::endl;

                ASSERT_TRUE(unzipper.extractEntry(entry.name, false));
                ASSERT_TRUE(helper::checkFileExists(
                    entry.name, 0 == i ? content1 : contentInFolder));

                ASSERT_TRUE(unzipper.extractEntry(entry.name, true));
                ASSERT_TRUE(helper::checkFileExists(
                    entry.name, 0 == i ? content1 : contentInFolder));

                ASSERT_FALSE(unzipper.extractEntry(entry.name, false));
                ASSERT_THAT(unzipper.error().message(),
                            testing::HasSubstr("already exists"));
                ASSERT_TRUE(helper::checkFileExists(
                    entry.name, 0 == i ? content1 : contentInFolder));
            }

            // Clean up
            for (const auto& entry : entries)
            {
                ASSERT_TRUE(helper::removeFileOrDir(entry.name));
            }

            unzipper.close();
        }

        // Extract single entry to specific dir
        {
            std::cout << "Extracting entries to specific dir: " << extractDir
                      << std::endl;

            ASSERT_TRUE(helper::removeFileOrDir(extractDir));
            Unzipper unzipper(zipFilename, password);
            auto entries = unzipper.entries();

            for (size_t i = 0; i < entries.size(); ++i)
            {
                const auto& entry = entries[i];
                std::cout << "  - Extracting " << entry.name << std::endl;

                ASSERT_TRUE(
                    unzipper.extractEntry(entry.name, extractDir, false));
                ASSERT_TRUE(helper::checkFileExists(extractDir + entry.name,
                                                    0 == i ? content1
                                                           : contentInFolder));

                ASSERT_TRUE(
                    unzipper.extractEntry(entry.name, extractDir, true));
                ASSERT_TRUE(helper::checkFileExists(extractDir + entry.name,
                                                    0 == i ? content1
                                                           : contentInFolder));

                ASSERT_FALSE(
                    unzipper.extractEntry(entry.name, extractDir, false));
                ASSERT_THAT(unzipper.error().message(),
                            testing::HasSubstr("already exists"));
                ASSERT_TRUE(helper::checkFileExists(extractDir + entry.name,
                                                    0 == i ? content1
                                                           : contentInFolder));
            }

            unzipper.close();

            // Clean up
            ASSERT_TRUE(helper::removeFileOrDir(extractDir));
        }

        // Extract all to specific dir
        {
            std::cout << "Extracting all to specific dir: " << extractDir
                      << std::endl;

            ASSERT_TRUE(helper::removeFileOrDir(extractDir));
            Unzipper unzipper(zipFilename, password);
            auto entries = unzipper.entries();

            ASSERT_TRUE(unzipper.extractAll(extractDir, false));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    extractDir + entries[i].name,
                    0 == i ? content1 : contentInFolder));
            }

            ASSERT_TRUE(unzipper.extractAll(extractDir, true));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    extractDir + entries[i].name,
                    0 == i ? content1 : contentInFolder));
            }

            ASSERT_FALSE(unzipper.extractAll(extractDir, false));
            ASSERT_THAT(unzipper.error().message(),
                        testing::HasSubstr("already exists"));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    extractDir + entries[i].name,
                    0 == i ? content1 : contentInFolder));
            }

            ASSERT_TRUE(helper::removeFileOrDir(extractDir));

            unzipper.close();
        }

        // Extract all to current dir
        {
            std::cout << "Extracting all to current dir" << std::endl;

            Unzipper unzipper(zipFilename, password);
            auto entries = unzipper.entries();

            ASSERT_TRUE(unzipper.extractAll(false));
            std::cout << unzipper.error().message() << std::endl;

            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    entries[i].name, 0 == i ? content1 : contentInFolder));
            }

            ASSERT_TRUE(unzipper.extractAll(true));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    entries[i].name, 0 == i ? content1 : contentInFolder));
            }

            ASSERT_FALSE(unzipper.extractAll(false));
            ASSERT_THAT(unzipper.error().message(),
                        testing::HasSubstr("already exists"));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    entries[i].name, 0 == i ? content1 : contentInFolder));
            }

            unzipper.close();

            // Clean up
            ASSERT_TRUE(helper::removeFileOrDir(zipFilename));
            ASSERT_TRUE(helper::removeFileOrDir(folder1));
            ASSERT_TRUE(helper::removeFileOrDir(emptyFolder));
            ASSERT_TRUE(helper::removeFileOrDir(file1));
            ASSERT_TRUE(helper::removeFileOrDir(fileNotInFolder));
        }
    }
}

//=============================================================================
// Test adding a large file with a password to trigger chunked CRC
// calculation.
//=============================================================================
TEST(ZipTests, LargeFileWithPassword)
{
    const std::string zip_filename = "ziptest_large.zip";
    const std::string large_filename = "large_test_file.dat";
    const std::string password = "verysecretpassword";
    const size_t file_size =
        110 * 1024 * 1024; // 110 MB to trigger chunked reading in getFileCrc

    // Clean up potential leftovers
    helper::removeFileOrDir(zip_filename);
    helper::removeFileOrDir(large_filename);

    // Create a large file
    {
        std::ofstream ofs(large_filename, std::ios::binary | std::ios::out);
        ASSERT_TRUE(ofs.is_open());
        // Fill with some data, 'A' for simplicity
        std::vector<char> buffer(1024 * 1024, 'A'); // 1MB buffer
        size_t bytes_written = 0;
        while (bytes_written < file_size)
        {
            size_t to_write =
                std::min(buffer.size(), file_size - bytes_written);
            ofs.write(buffer.data(), std::streamsize(to_write));
            ASSERT_TRUE(ofs.good());
            bytes_written += to_write;
        }
        ofs.flush();
        ASSERT_TRUE(ofs.good());
        ofs.close();
    }
    ASSERT_TRUE(Path::exist(large_filename));
    ASSERT_EQ(Path::getFileSize(large_filename), file_size);

    // Create zipper with password and add the large file
    {
        Zipper zipper(zip_filename, password);
        ASSERT_TRUE(zipper.isOpen());
        std::ifstream ifs(large_filename, std::ios::binary);
        ASSERT_TRUE(ifs.is_open());
        ASSERT_TRUE(zipper.add(ifs, large_filename));
        ifs.close();
        zipper.close();
        ASSERT_FALSE(zipper.isOpen());
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
    }

    // Verify the zip file
    ASSERT_TRUE(Path::exist(zip_filename));
    {
        zipper::Unzipper unzipper(zip_filename, password);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        std::vector<zipper::ZipEntry> entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1u);
        ASSERT_STREQ(entries[0].name.c_str(), large_filename.c_str());
        ASSERT_EQ(entries[0].uncompressed_size, file_size);
        unzipper.close();
    }

    // Clean up
    helper::removeFileOrDir(zip_filename);
    helper::removeFileOrDir(large_filename);
}