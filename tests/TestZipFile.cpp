#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define protected public
#define private public
#include "Zipper/Unzipper.hpp"
#include "Zipper/Zipper.hpp"
#undef protected
#undef private

#include "TestHelper.hpp"

using namespace zipper;

//=============================================================================
// Helper functions for tests
//=============================================================================
namespace helper {

/**
 * @brief Creates a file with content and adds it to the zipper.
 * @param[in] p_zipper The zipper instance.
 * @param[in] p_file_path Path of the temporary file to create.
 * @param[in] p_content Content to write in the file.
 * @param[in] p_entry_path Path of the entry in the zip archive.
 * @param[in] p_flags Compression flags for the file.
 * @return true if successful, Unzipper::OverwriteMode::Overwrite otherwise.
 */
static bool zipAddFile(Zipper& p_zipper,
                       const std::string& p_file_path,
                       const std::string& p_content,
                       const std::string& p_entry_path,
                       Zipper::ZipFlags p_flags = Zipper::ZipFlags::Better)
{
    std::string dir_name = Path::dirName(p_file_path);
    if (!dir_name.empty())
    {
        if (!Path::createDir(dir_name))
        {
            std::cerr << "Failed to create directory: " << dir_name
                      << std::endl;
            return false;
        }
    }

    if (!helper::createFile(p_file_path, p_content))
    {
        std::cerr << "Failed to create file: " << p_file_path << std::endl;
        return false;
    }

    std::ifstream ifs(p_file_path);
    bool res = p_zipper.add(ifs, p_entry_path, p_flags);
    if (!res)
    {
        std::cerr << "Failed to add file: " << p_file_path << std::endl;
    }
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
    ASSERT_FALSE(zipper1.isOpened());
    zipper1.close();
    ASSERT_FALSE(zipper1.isOpened());
    zipper1.reopen();
    ASSERT_FALSE(zipper1.isOpened());

    // Opening from a non existing zip file.
    zipper1.open("ziptest_nominal.zip");
    ASSERT_TRUE(zipper1.isOpened());
    zipper1.close();
    ASSERT_FALSE(zipper1.isOpened());

    // Opening from an existing zip file.
    ASSERT_TRUE(Path::exist("ziptest_nominal.zip"));
    ASSERT_TRUE(Path::isFile("ziptest_nominal.zip"));
    Zipper zipper2("ziptest_nominal.zip");
    ASSERT_TRUE(zipper2.isOpened());
    zipper2.close();
    ASSERT_FALSE(zipper2.isOpened());
    zipper2.reopen();
    ASSERT_TRUE(zipper2.isOpened());
    zipper2.close();
    ASSERT_FALSE(zipper2.isOpened());

    // Opening an existent zipfile.
    ASSERT_TRUE(Path::exist("ziptest_nominal.zip"));
    ASSERT_TRUE(Path::isFile("ziptest_nominal.zip"));
    Unzipper unzipper2("ziptest_nominal.zip");
    ASSERT_TRUE(unzipper2.isOpened());
    unzipper2.close();
    ASSERT_FALSE(unzipper2.isOpened());
    // TODO: to implement
    // unzipper2.open();
    // ASSERT_TRUE(zipper2.isOpened());
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
            ASSERT_TRUE(zipper.isOpened());
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }

        // Verify content. Check if file1 is in the zip archive.
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_TRUE(unzipper.isOpened());
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            auto entries = unzipper.entries();
            unzipper.close();
            ASSERT_EQ(entries.size(), 1u);
            ASSERT_EQ(entries[0].name, file1);
        }

        // Reopen with Append
        {
            Zipper zipper(zip_filename, password, Zipper::OpenFlags::Append);
            ASSERT_TRUE(zipper.isOpened());
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            ASSERT_TRUE(helper::zipAddFile(zipper, file3, content3, file3));
            ASSERT_FALSE(zipper.error()) << zipper.error().message();
            zipper.close();
        }

        // Verify content (file1, file3)
        {
            Unzipper unzipper(zip_filename, password);
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            ASSERT_TRUE(unzipper.isOpened());
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
    const std::string zip_poc0 = PWD "/issues/poc0.zip";
    const std::string zip_poc1 = PWD "/issues/poc1.zip";

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
#    if defined(__APPLE__)
    const std::string protected_path = "/System/Library/";
#    elif defined(__linux__)
    const std::string protected_path = "/root/";
#    else
#        error "Unsupported platform"
#    endif

    try
    {
        Zipper zipper(protected_path + "no_permission_test.zip");
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
        Unzipper unzipper(protected_path + "no_permission_test.zip");
        FAIL() << "Expected std::runtime_error for permission denied";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_THAT(e.what(), testing::HasSubstr("Permission denied"));
    }

    // Try adding a protected file to the zip archive.
    {
        Zipper zipper("foo.zip");
        ASSERT_FALSE(zipper.add(protected_path));
        ASSERT_THAT(zipper.error().message(),
                    testing::HasSubstr("Permission denied"));
        zipper.close();
    }

    // Try extracting a file to a protected directory.
    {
        Zipper zipper("foo.zip");
        helper::zipAddFile(zipper, "file.txt", "content", "file.txt");
        zipper.close();

        Unzipper unzipper("foo.zip");
        ASSERT_FALSE(unzipper.extractAll(
            protected_path, Unzipper::OverwriteMode::DoNotOverwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Permission denied"));
        unzipper.close();
    }

    helper::removeFileOrDir("foo.zip");
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
    // Append to a dummy zip file: allowed
    Zipper zipper1(PWD "/issues/dummy.zip", Zipper::OpenFlags::Append);
    ASSERT_TRUE(zipper1.isOpened());
    zipper1.close();

    // Open a dummy zip file: allowed
    Unzipper unzipper1(PWD "/issues/dummy.zip");
    ASSERT_TRUE(unzipper1.isOpened());
    unzipper1.close();

    //
    Zipper zipper2("foo.zip");
    ASSERT_TRUE(zipper2.isOpened());

    // Empty file content: allowed
    ASSERT_TRUE(helper::zipAddFile(zipper2, "dummy.txt", "", "dummy.txt"));

    // Empty entry zip name: forbidden
    ASSERT_FALSE(helper::zipAddFile(zipper2, "dummy.txt", "", ""));
    ASSERT_THAT(zipper2.error().message(),
                testing::HasSubstr("cannot be empty"));

    // Add a dummy directory: allowed
    helper::createDir("dummy_dir");
    // TODO
    // ASSERT_TRUE(zipper2.addDir("dummy_dir"));
    zipper2.close();

    // Clean up
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
                        testing::HasSubstr("Failed opening file"));

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

                ASSERT_TRUE(unzipper.extract(
                    entry.name, Unzipper::OverwriteMode::Overwrite));
                ASSERT_TRUE(helper::checkFileExists(
                    entry.name, 0 == i ? content1 : contentInFolder));

                ASSERT_TRUE(unzipper.extract(
                    entry.name, Unzipper::OverwriteMode::Overwrite));
                ASSERT_TRUE(helper::checkFileExists(
                    entry.name, 0 == i ? content1 : contentInFolder));

                ASSERT_FALSE(unzipper.extract(
                    entry.name, Unzipper::OverwriteMode::DoNotOverwrite));
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
                    unzipper.extract(entry.name,
                                     extractDir,
                                     Unzipper::OverwriteMode::Overwrite));
                ASSERT_TRUE(helper::checkFileExists(extractDir + entry.name,
                                                    0 == i ? content1
                                                           : contentInFolder));

                ASSERT_TRUE(
                    unzipper.extract(entry.name,
                                     extractDir,
                                     Unzipper::OverwriteMode::Overwrite));
                ASSERT_TRUE(helper::checkFileExists(extractDir + entry.name,
                                                    0 == i ? content1
                                                           : contentInFolder));

                ASSERT_FALSE(
                    unzipper.extract(entry.name,
                                     extractDir,
                                     Unzipper::OverwriteMode::DoNotOverwrite));
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

            ASSERT_TRUE(unzipper.extractAll(
                extractDir, Unzipper::OverwriteMode::Overwrite));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    extractDir + entries[i].name,
                    0 == i ? content1 : contentInFolder));
            }

            ASSERT_TRUE(unzipper.extractAll(
                extractDir, Unzipper::OverwriteMode::Overwrite));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    extractDir + entries[i].name,
                    0 == i ? content1 : contentInFolder));
            }

            ASSERT_FALSE(unzipper.extractAll(
                extractDir, Unzipper::OverwriteMode::DoNotOverwrite));
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

            ASSERT_TRUE(
                unzipper.extractAll(Unzipper::OverwriteMode::DoNotOverwrite));
            std::cout << unzipper.error().message() << std::endl;

            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    entries[i].name, 0 == i ? content1 : contentInFolder));
            }

            ASSERT_TRUE(
                unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ASSERT_TRUE(helper::checkFileExists(
                    entries[i].name, 0 == i ? content1 : contentInFolder));
            }

            ASSERT_FALSE(
                unzipper.extractAll(Unzipper::OverwriteMode::DoNotOverwrite));
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
// Test adding a file with a relative path
//=============================================================================
TEST(ZipTests, AddFileWithRelativePath)
{
    const std::string zip_filename = "ziptest_relative.zip";
    const std::string input_entry_path = "foo/../Test1";
    const std::string expected_entry_path = "Test1";

    Zipper zipper(zip_filename);
    helper::zipAddFile(zipper, "foo.txt", "content", input_entry_path);
    zipper.close();

    Unzipper unzipper(zip_filename);
    auto entries = unzipper.entries();
    unzipper.close();

    // Check normalized path is used for entry name
    ASSERT_EQ(entries.size(), 1u);
    ASSERT_STREQ(entries[0].name.c_str(), expected_entry_path.c_str());

    // Clean up
    helper::removeFileOrDir(zip_filename);
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
        ASSERT_TRUE(zipper.isOpened());
        std::ifstream ifs(large_filename, std::ios::binary);
        ASSERT_TRUE(ifs.is_open());
        ASSERT_TRUE(zipper.add(ifs, large_filename));
        ifs.close();
        zipper.close();
        ASSERT_FALSE(zipper.isOpened());
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
    ASSERT_TRUE(helper::removeFileOrDir(zip_filename));
    ASSERT_TRUE(helper::removeFileOrDir(large_filename));
}

//=============================================================================
// Test extracting from a closed zip file
//=============================================================================
TEST(ZipperFileOps, AddAndExtractClosed)
{
    const std::string zipFilename = "ziptest_add.zip";
    const std::string file1 = "file1_add.txt";

    Zipper zipper(zipFilename);
    ASSERT_TRUE(helper::zipAddFile(zipper, file1, "foo", file1));
    zipper.close();

    ASSERT_FALSE(helper::zipAddFile(zipper, file1, "foo", file1));
    ASSERT_THAT(zipper.error().message(),
                testing::HasSubstr("Zip archive is not opened"));
    ASSERT_FALSE(zipper.add(file1));
    ASSERT_THAT(zipper.error().message(),
                testing::HasSubstr("Zip archive is not opened"));

    Unzipper unzipper(zipFilename);
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), file1.c_str());
    unzipper.close();

    ASSERT_EQ(unzipper.entries().size(), 0u);
    ASSERT_FALSE(unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
    ASSERT_THAT(unzipper.error().message(),
                testing::HasSubstr("Zip archive is not opened"));
}

//=============================================================================
//
//=============================================================================
TEST(ZipperFileOps, ExtractBadPassword)
{
    const std::string zipFilename = "ziptest_add.zip";
    const std::string file1 = "file1_add.txt";

    Zipper zipper(zipFilename, "correct_pass");
    helper::zipAddFile(zipper, file1, "foo", file1);
    zipper.close();

    Unzipper unzipper(zipFilename, "bad_pass");
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), file1.c_str());
    ASSERT_FALSE(unzipper.extract(file1, Unzipper::OverwriteMode::Overwrite));
    ASSERT_THAT(unzipper.error().message(), testing::HasSubstr("Bad password"));
    ASSERT_FALSE(unzipper.extract(file1, Unzipper::OverwriteMode::Overwrite));
    ASSERT_THAT(unzipper.error().message(), testing::HasSubstr("Bad password"));
    ASSERT_FALSE(
        unzipper.extract(file1, Unzipper::OverwriteMode::DoNotOverwrite));
    ASSERT_THAT(unzipper.error().message(),
                testing::HasSubstr("already exists"));
    ASSERT_FALSE(unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
    ASSERT_THAT(unzipper.error().message(), testing::HasSubstr("Bad password"));
    ASSERT_FALSE(unzipper.extractAll(Unzipper::OverwriteMode::DoNotOverwrite));
    ASSERT_THAT(unzipper.error().message(),
                testing::HasSubstr("already exists"));
    unzipper.close();

    ASSERT_FALSE(helper::checkFileDoesNotExist(file1));

    ASSERT_TRUE(helper::removeFileOrDir(zipFilename));
    ASSERT_TRUE(helper::removeFileOrDir(file1));
}

//=============================================================================
// Test Suite for Unzipper error handling
//=============================================================================
TEST(UnzipperFileOps, ErrorHandling)
{
    const std::string zipFilename = "ziptest_errors.zip";
    const std::string file1 = "file1_errors.txt";
    const std::string content1 = "content errors 1";
    const std::string file2 = "file2_errors.txt";
    const std::string content2 = "content errors 2";

    // Create a zip file with two entries
    {
        Zipper zipper(zipFilename);
        ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
        ASSERT_TRUE(helper::zipAddFile(zipper, file2, content2, file2));
        zipper.close();
    }

    // Test invalid entry name
    {
        Unzipper unzipper(zipFilename);
        ASSERT_FALSE(unzipper.extract("non_existent_entry.txt",
                                      Unzipper::OverwriteMode::DoNotOverwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Unknown entry name"));
        unzipper.close();
    }

    // Test extract to stream with invalid entry
    {
        Unzipper unzipper(zipFilename);
        std::stringstream output;
        ASSERT_FALSE(unzipper.extract("non_existent_entry.txt", output));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Unknown entry name"));
        unzipper.close();
    }

    // Test extract to memory with invalid entry
    {
        Unzipper unzipper(zipFilename);
        std::vector<unsigned char> output;
        ASSERT_FALSE(unzipper.extract("non_existent_entry.txt", output));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Unknown entry name"));
        unzipper.close();
    }

    // Test extract all with alternative names
    {
        Unzipper unzipper(zipFilename);
        std::map<std::string, std::string> alternative_names;
        alternative_names[file1] = "renamed1.txt";
        alternative_names[file2] = "renamed2.txt";
        ASSERT_TRUE(
            unzipper.extractAll("extract_dir",
                                alternative_names,
                                Unzipper::OverwriteMode::DoNotOverwrite));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed1.txt", content1));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed2.txt", content2));
        unzipper.close();
        helper::removeFileOrDir("extract_dir");
    }

    // Test extract all to current directory
    {
        Unzipper unzipper(zipFilename);
        ASSERT_TRUE(unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
        ASSERT_TRUE(helper::checkFileExists(file1, content1));
        ASSERT_TRUE(helper::checkFileExists(file2, content2));
        unzipper.close();
        helper::removeFileOrDir(file1);
        helper::removeFileOrDir(file2);
    }

    // Test extract all to specific directory
    {
        Unzipper unzipper(zipFilename);
        ASSERT_TRUE(unzipper.extractAll(
            "extract_dir", Unzipper::OverwriteMode::DoNotOverwrite));
        ASSERT_TRUE(helper::checkFileExists("extract_dir/" + file1, content1));
        ASSERT_TRUE(helper::checkFileExists("extract_dir/" + file2, content2));
        unzipper.close();
        helper::removeFileOrDir("extract_dir");
    }

    // Test checkValid() with uninitialized Unzipper
    {
        Unzipper unzipper;
        ASSERT_FALSE(unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("not initialized"));
    }

    // Test checkValid() with closed Unzipper
    {
        Unzipper unzipper(zipFilename);
        unzipper.close();
        ASSERT_FALSE(unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("not opened"));
    }

    // Clean up
    helper::removeFileOrDir(zipFilename);
}

//=============================================================================
// Test Suite for Unzipper with empty zip files
//=============================================================================
TEST(UnzipperFileOps, EmptyZipHandling)
{
    const std::string emptyZipFilename = "empty_zip.zip";

    // Create an empty zip file
    {
        Zipper zipper(emptyZipFilename);
        zipper.close();
    }

    // Test entries() on empty zip
    {
        Unzipper unzipper(emptyZipFilename);
        auto entries = unzipper.entries();
        ASSERT_TRUE(entries.empty());
        unzipper.close();
    }

    // Test extractAll() on empty zip
    {
        Unzipper unzipper(emptyZipFilename);
        ASSERT_FALSE(unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Failed going to first entry"));
        unzipper.close();
    }

    // Clean up
    helper::removeFileOrDir(emptyZipFilename);
}

//=============================================================================
// Test Suite for Unzipper with password protected files
//=============================================================================
TEST(UnzipperFileOps, PasswordProtectedFiles)
{
    const std::string zipFilename = "password_protected.zip";
    const std::string file1 = "file1_pwd.txt";
    const std::string content1 = "content pwd 1";
    const std::string password = "test_password";

    // Create a password protected zip file
    {
        Zipper zipper(zipFilename, password);
        ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
        zipper.close();
    }

    // Test with correct password
    {
        Unzipper unzipper(zipFilename, password);
        std::vector<unsigned char> output;
        ASSERT_TRUE(unzipper.extract(file1, output));
        std::string extracted(output.begin(), output.end());
        ASSERT_EQ(extracted, content1);
        unzipper.close();
    }

    // Test with wrong password
    {
        Unzipper unzipper(zipFilename, "wrong_password");
        std::vector<unsigned char> output;
        ASSERT_FALSE(unzipper.extract(file1, output));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Bad password"));
        unzipper.close();
    }

    // Clean up
    helper::removeFileOrDir(zipFilename);
}

//=============================================================================
// Test Suite for different compression flags
//=============================================================================
TEST(ZipperFileOps, CompressionFlags)
{
    const std::string zipFilename = "ziptest_compression.zip";
    const std::string file1 = "file1_comp.txt";
    const std::string file2 = "file2_comp.txt";
    const std::string file3 = "file3_comp.txt";
    const std::string file4 = "file4_comp.txt";
    const std::string content =
        "This is a test content for compression testing. "
        "We need enough content to see compression effects. "
        "Repeating this text multiple times to ensure we "
        "have enough data to compress... ";

    // Create a file with repeated content to ensure compression effects
    std::string repeatedContent;
    for (int i = 0; i < 100;
         ++i) // Increased from 10 to 100 for better compression effects
    {
        repeatedContent += content;
    }

    // Test different compression flags
    {
        Zipper zipper(zipFilename);
        ASSERT_TRUE(zipper.isOpened());

        // Store (no compression)
        ASSERT_TRUE(helper::zipAddFile(
            zipper, file1, repeatedContent, file1, Zipper::ZipFlags::Store));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();

        // Faster compression
        ASSERT_TRUE(helper::zipAddFile(
            zipper, file2, repeatedContent, file2, Zipper::ZipFlags::Faster));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();

        // Medium compression
        ASSERT_TRUE(helper::zipAddFile(
            zipper, file3, repeatedContent, file3, Zipper::ZipFlags::Medium));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();

        // Better compression
        ASSERT_TRUE(helper::zipAddFile(
            zipper, file4, repeatedContent, file4, Zipper::ZipFlags::Better));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();

        zipper.close();
    }

    // Verify the compressed files
    {
        Unzipper unzipper(zipFilename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 4u);

        // Verify each file's content
        for (const auto& entry : entries)
        {
            std::vector<unsigned char> output;
            ASSERT_TRUE(unzipper.extract(entry.name, output));
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            std::string extracted(output.begin(), output.end());
            ASSERT_EQ(extracted, repeatedContent);
        }

        // Verify Store has no compression (compressed_size ==
        // uncompressed_size)
        ASSERT_EQ(entries[0].compressed_size, entries[0].uncompressed_size);

        // Verify that compressed sizes are different from Store
        ASSERT_NE(entries[1].compressed_size, entries[0].compressed_size);
        ASSERT_NE(entries[2].compressed_size, entries[0].compressed_size);
        ASSERT_NE(entries[3].compressed_size, entries[0].compressed_size);

        // Verify that compressed sizes are smaller than uncompressed size
        for (size_t i = 1; i < entries.size(); ++i)
        {
            ASSERT_LT(entries[i].compressed_size, entries[i].uncompressed_size);
        }

        // Verify that compression levels are ordered (Better <= Medium <=
        // Faster)
        ASSERT_LE(entries[3].compressed_size, entries[2].compressed_size);
        ASSERT_LE(entries[2].compressed_size, entries[1].compressed_size);

        unzipper.close();
    }

    // Clean up
    ASSERT_TRUE(helper::removeFileOrDir(zipFilename));
}

//=============================================================================
// Test Suite for compression flags with hierarchy
//=============================================================================
TEST(ZipperFileOps, CompressionFlagsWithHierarchy)
{
    const std::string zipFilename = "ziptest_compression_hierarchy.zip";
    const std::string folder1 = "folder1_comp/";
    const std::string folder2 = "folder2_comp/";
    const std::string file1 = folder1 + "file1_comp.txt";
    const std::string file2 = folder2 + "file2_comp.txt";
    const std::string content =
        "This is a test content for compression testing with hierarchy. "
        "We need enough content to see compression effects. "
        "Repeating this text multiple times to ensure we "
        "have enough data to compress... ";

    // Create a file with repeated content
    std::string repeatedContent;
    for (int i = 0; i < 10; ++i)
    {
        repeatedContent += content;
    }

    // Create directories
    ASSERT_TRUE(helper::createDir(folder1));
    ASSERT_TRUE(helper::createDir(folder2));

    // Test different compression flags with hierarchy
    {
        Zipper zipper(zipFilename);
        ASSERT_TRUE(zipper.isOpened());

        // Store with hierarchy
        ASSERT_TRUE(helper::zipAddFile(zipper,
                                       file1,
                                       repeatedContent,
                                       file1,
                                       Zipper::ZipFlags::Store |
                                           Zipper::ZipFlags::SaveHierarchy));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();

        // Better with hierarchy
        ASSERT_TRUE(helper::zipAddFile(zipper,
                                       file2,
                                       repeatedContent,
                                       file2,
                                       Zipper::ZipFlags::Better |
                                           Zipper::ZipFlags::SaveHierarchy));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();

        zipper.close();
    }

    // Verify the compressed files with hierarchy
    {
        Unzipper unzipper(zipFilename);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 2u);

        // Verify each file's content and path
        for (const auto& entry : entries)
        {
            std::vector<unsigned char> output;
            ASSERT_TRUE(unzipper.extract(entry.name, output));
            ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
            std::string extracted(output.begin(), output.end());
            ASSERT_EQ(extracted, repeatedContent);
            ASSERT_TRUE(entry.name.find("folder") != std::string::npos);
        }

        // Verify compression differences
        ASSERT_NE(entries[0].compressed_size, entries[1].compressed_size);
        ASSERT_EQ(entries[0].compressed_size, entries[0].uncompressed_size);
        ASSERT_LT(entries[1].compressed_size, entries[0].compressed_size);

        unzipper.close();
    }

    // Clean up
    ASSERT_TRUE(helper::removeFileOrDir(zipFilename));
    ASSERT_TRUE(helper::removeFileOrDir(folder1));
    ASSERT_TRUE(helper::removeFileOrDir(folder2));
}

//=============================================================================
// Test Suite for extractGlob functionality
//=============================================================================
TEST(UnzipperFileOps, ExtractGlob)
{
    const std::string zipFilename = "ziptest_glob.zip";
    const std::string file1 = "test1.txt";
    const std::string file2 = "test2.txt";
    const std::string file3 = "doc/test3.txt";
    const std::string file4 = "doc/test4.txt";
    const std::string content1 = "content 1";
    const std::string content2 = "content 2";
    const std::string content3 = "content 3";
    const std::string content4 = "content 4";

    // Create a zip file with multiple entries
    {
        Zipper zipper(zipFilename);
        ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
        ASSERT_TRUE(helper::zipAddFile(zipper, file2, content2, file2));
        ASSERT_TRUE(helper::zipAddFile(zipper, file3, content3, file3));
        ASSERT_TRUE(helper::zipAddFile(zipper, file4, content4, file4));
        zipper.close();
    }

    // Test glob pattern matching all files
    {
        Unzipper unzipper(zipFilename);
        ASSERT_TRUE(
            unzipper.extractGlob("*", Unzipper::OverwriteMode::Overwrite));
        ASSERT_TRUE(helper::checkFileExists(file1, content1));
        ASSERT_TRUE(helper::checkFileExists(file2, content2));
        ASSERT_TRUE(helper::checkFileExists(file3, content3));
        ASSERT_TRUE(helper::checkFileExists(file4, content4));
        unzipper.close();

        // Clean up
        ASSERT_TRUE(helper::removeFileOrDir(file1));
        ASSERT_TRUE(helper::removeFileOrDir(file2));
        ASSERT_TRUE(helper::removeFileOrDir(file3));
        ASSERT_TRUE(helper::removeFileOrDir(file4));
        ASSERT_TRUE(helper::removeFileOrDir("doc"));
    }

    // Test glob pattern matching specific files
    {
        Unzipper unzipper(zipFilename);
        ASSERT_TRUE(unzipper.extractGlob("test*.txt",
                                         Unzipper::OverwriteMode::Overwrite));
        ASSERT_TRUE(helper::checkFileExists(file1, content1));
        ASSERT_TRUE(helper::checkFileExists(file2, content2));
        ASSERT_FALSE(helper::checkFileExists(file3));
        ASSERT_FALSE(helper::checkFileExists(file4));
        unzipper.close();

        // Clean up
        ASSERT_TRUE(helper::removeFileOrDir(file1));
        ASSERT_TRUE(helper::removeFileOrDir(file2));
        ASSERT_TRUE(helper::removeFileOrDir(file3));
        ASSERT_TRUE(helper::removeFileOrDir(file4));
        ASSERT_TRUE(helper::removeFileOrDir("doc"));
    }

    // Test glob pattern matching files in specific directory
    {
        Unzipper unzipper(zipFilename);
        ASSERT_TRUE(
            unzipper.extractGlob("doc/*", Unzipper::OverwriteMode::Overwrite));
        ASSERT_FALSE(helper::checkFileExists(file1));
        ASSERT_FALSE(helper::checkFileExists(file2));
        ASSERT_TRUE(helper::checkFileExists(file3, content3));
        ASSERT_TRUE(helper::checkFileExists(file4, content4));
        unzipper.close();

        // Clean up
        ASSERT_TRUE(helper::removeFileOrDir(file1));
        ASSERT_TRUE(helper::removeFileOrDir(file2));
        ASSERT_TRUE(helper::removeFileOrDir(file3));
        ASSERT_TRUE(helper::removeFileOrDir(file4));
        ASSERT_TRUE(helper::removeFileOrDir("doc"));
    }

    // Test glob pattern with alternative names
    {
        ASSERT_TRUE(helper::removeFileOrDir("extract_dir"));
        Unzipper unzipper(zipFilename);
        std::map<std::string, std::string> alternative_names;
        alternative_names[file1] = "renamed1.txt";
        alternative_names[file2] = "renamed2.txt";
        ASSERT_TRUE(unzipper.extractGlob("test*.txt",
                                         "extract_dir",
                                         alternative_names,
                                         Unzipper::OverwriteMode::Overwrite));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed1.txt", content1));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed2.txt", content2));
        unzipper.close();

        // Clean up
        ASSERT_TRUE(helper::removeFileOrDir("extract_dir"));
    }

    // Clean up
    ASSERT_TRUE(helper::removeFileOrDir(zipFilename));
}

//=============================================================================
// Test Suite for extractAll with alternative names
//=============================================================================
TEST(UnzipperFileOps, ExtractAllWithAlternativeNames)
{
    const std::string zipFilename = "ziptest_alternative.zip";
    const std::string file1 = "test1.txt";
    const std::string file2 = "test2.txt";
    const std::string file3 = "doc/test3.txt";
    const std::string content1 = "content 1";
    const std::string content2 = "content 2";
    const std::string content3 = "content 3";

    // Create a zip file with multiple entries
    {
        Zipper zipper(zipFilename);
        ASSERT_TRUE(helper::zipAddFile(zipper, file1, content1, file1));
        ASSERT_TRUE(helper::zipAddFile(zipper, file2, content2, file2));
        ASSERT_TRUE(helper::zipAddFile(zipper, file3, content3, file3));
        zipper.close();
    }

    // Test extractAll with alternative names to current directory
    {
        Unzipper unzipper(zipFilename);
        std::map<std::string, std::string> alternative_names;
        alternative_names[file1] = "renamed1.txt";
        alternative_names[file2] = "renamed2.txt";
        alternative_names[file3] = "renamed3.txt";
        ASSERT_TRUE(unzipper.extractAll(
            "", alternative_names, Unzipper::OverwriteMode::Overwrite));
        ASSERT_TRUE(helper::checkFileExists("renamed1.txt", content1));
        ASSERT_TRUE(helper::checkFileExists("renamed2.txt", content2));
        ASSERT_TRUE(helper::checkFileExists("renamed3.txt", content3));
        unzipper.close();

        // Clean up
        helper::removeFileOrDir("renamed1.txt");
        helper::removeFileOrDir("renamed2.txt");
        helper::removeFileOrDir("renamed3.txt");
    }

    // Test extractAll with alternative names to specific directory
    {
        Unzipper unzipper(zipFilename);
        std::map<std::string, std::string> alternative_names;
        alternative_names[file1] = "renamed1.txt";
        alternative_names[file2] = "renamed2.txt";
        alternative_names[file3] = "renamed3.txt";
        ASSERT_TRUE(unzipper.extractAll("extract_dir",
                                        alternative_names,
                                        Unzipper::OverwriteMode::Overwrite));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed1.txt", content1));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed2.txt", content2));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed3.txt", content3));
        unzipper.close();

        // Clean up
        helper::removeFileOrDir("extract_dir");
    }

    // Test extractAll with partial alternative names
    {
        Unzipper unzipper(zipFilename);
        std::map<std::string, std::string> alternative_names;
        alternative_names[file1] = "renamed1.txt";
        // file2 and file3 will keep their original names
        ASSERT_TRUE(unzipper.extractAll("extract_dir",
                                        alternative_names,
                                        Unzipper::OverwriteMode::Overwrite));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/renamed1.txt", content1));
        ASSERT_TRUE(helper::checkFileExists("extract_dir/test2.txt", content2));
        ASSERT_TRUE(
            helper::checkFileExists("extract_dir/doc/test3.txt", content3));
        unzipper.close();

        // Clean up
        helper::removeFileOrDir("extract_dir");
    }

    // Clean up
    helper::removeFileOrDir(zipFilename);
}