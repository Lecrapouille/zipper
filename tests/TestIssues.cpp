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
// https://github.com/Lecrapouille/zipper/issues/5
//=============================================================================
TEST(MemoryZipTests, Issue05_1)
{
    zipper::Unzipper unzipper(PWD "/issues/issue_05_1.zip");
    std::string temp_dir = "issue_05_1/";
    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));

    // Check the zip file given in the GitHub issue
    std::vector<zipper::ZipEntry> entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 3u);
    ASSERT_STREQ(entries[0].name.c_str(), "sim.sedml");
    ASSERT_STREQ(entries[1].name.c_str(), "model.xml");
    ASSERT_STREQ(entries[2].name.c_str(), "manifest.xml");

    // Check can be extracted
    ASSERT_TRUE(
        unzipper.extractAll(temp_dir, Unzipper::OverwriteMode::DoNotOverwrite));
    ASSERT_TRUE(helper::checkFileExists(temp_dir + "sim.sedml", ""));
    ASSERT_TRUE(helper::checkFileExists(temp_dir + "model.xml", ""));
    ASSERT_TRUE(helper::checkFileExists(temp_dir + "manifest.xml"));

    // Check cannot be extracted a second time, by security: files already
    // exist, if flag is set.
    std::string error = "Security Error: '" +
                        Path::toNativeSeparators(temp_dir + "manifest.xml") +
                        "' already exists and would have been replaced!";
    ASSERT_FALSE(
        unzipper.extractAll(temp_dir, Unzipper::OverwriteMode::DoNotOverwrite));
    ASSERT_STREQ(unzipper.error().message().c_str(), error.c_str());

    // Check cannot be extracted, by security: files already exist.
    ASSERT_FALSE(unzipper.extractAll(temp_dir)); // Implicit: false argument
    ASSERT_STREQ(unzipper.error().message().c_str(), error.c_str());

    // Check can be extracted, when security is disabled.
    ASSERT_TRUE(
        unzipper.extractAll(temp_dir, Unzipper::OverwriteMode::Overwrite));
    ASSERT_TRUE(helper::checkFileExists(temp_dir + "sim.sedml", ""));
    ASSERT_TRUE(helper::checkFileExists(temp_dir + "model.xml", ""));
    ASSERT_TRUE(helper::checkFileExists(temp_dir + "manifest.xml"));

    unzipper.close();
    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));
}

//=============================================================================
// https://github.com/Lecrapouille/zipper/issues/5
//=============================================================================
TEST(MemoryZipTests, Issue05_nopassword)
{
    zipper::Unzipper unzipper(PWD "/issues/issue_05_nopassword.zip");
    ASSERT_TRUE(helper::removeFileOrDir("issue_05"));

    // Check the zip file given in the GitHub issue
    std::vector<zipper::ZipEntry> entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 5u);
    ASSERT_STREQ(entries[0].name.c_str(), "issue_05/");
    ASSERT_STREQ(entries[1].name.c_str(), "issue_05/Nouveau dossier/");
    ASSERT_STREQ(entries[2].name.c_str(), "issue_05/Nouveau fichier vide");
    ASSERT_STREQ(entries[3].name.c_str(), "issue_05/foo/");
    ASSERT_STREQ(entries[4].name.c_str(), "issue_05/foo/bar");

    // Check can be extracted
    ASSERT_TRUE(
        unzipper.extractAll(".", Unzipper::OverwriteMode::DoNotOverwrite));
    unzipper.close();

    // Check files and directories were created
    ASSERT_TRUE(helper::checkDirExists("issue_05/"));
    ASSERT_TRUE(helper::checkDirExists("issue_05/Nouveau dossier/"));
    ASSERT_TRUE(helper::checkFileExists("issue_05/Nouveau fichier vide", ""));
    ASSERT_TRUE(helper::checkDirExists("issue_05/foo/"));
    ASSERT_TRUE(helper::checkFileExists("issue_05/foo/bar", ""));

    ASSERT_TRUE(helper::removeFileOrDir("issue_05"));
}

//=============================================================================
// https://github.com/Lecrapouille/zipper/issues/5
//=============================================================================
TEST(MemoryZipTests, Issue05_password)
{
    zipper::Unzipper unzipper(PWD "/issues/issue_05_password.zip", "1234");
    ASSERT_TRUE(helper::removeFileOrDir("issue_05"));

    // Check the zip file given in the GitHub issue
    std::vector<zipper::ZipEntry> entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 5u);
    ASSERT_STREQ(entries[0].name.c_str(), "issue_05/");
    ASSERT_STREQ(entries[1].name.c_str(), "issue_05/Nouveau dossier/");
    ASSERT_STREQ(entries[2].name.c_str(), "issue_05/Nouveau fichier vide");
    ASSERT_STREQ(entries[3].name.c_str(), "issue_05/foo/");
    ASSERT_STREQ(entries[4].name.c_str(), "issue_05/foo/bar");

    // Check can be extracted
    ASSERT_TRUE(
        unzipper.extractAll(".", Unzipper::OverwriteMode::DoNotOverwrite));
    unzipper.close();

    // Check files and directories were created
    ASSERT_TRUE(helper::checkDirExists("issue_05/"));
    ASSERT_TRUE(helper::checkDirExists("issue_05/Nouveau dossier/"));
    ASSERT_TRUE(helper::checkFileExists("issue_05/Nouveau fichier vide", ""));
    ASSERT_TRUE(helper::checkDirExists("issue_05/foo/"));
    ASSERT_TRUE(helper::checkFileExists("issue_05/foo/bar", ""));

    ASSERT_TRUE(helper::removeFileOrDir("issue_05"));
}

//=============================================================================
// Tests for issue #21: Problems with directory paths ending with '/'
// https://github.com/sebastiandev/zipper/issues/21
//=============================================================================
TEST(ZipTests, Issue21)
{
    // Create folder
    ASSERT_TRUE(helper::removeFileOrDir("data"));
    ASSERT_TRUE(helper::createDir("data/somefolder/"));
    ASSERT_TRUE(helper::createFile("data/somefolder/test.txt",
                                   "test file2 compression"));

    // Test with the '/' with and without the option Zipper::SaveHierarchy
    {
        Zipper zipper("ziptest.zip", Zipper::OpenFlags::Overwrite);
        EXPECT_EQ(zipper.add("data/somefolder/", Zipper::SaveHierarchy), true);
        EXPECT_EQ(zipper.add("data/somefolder/", Zipper::SaveHierarchy), true);
        EXPECT_EQ(zipper.add("data/somefolder/"), true);
        EXPECT_EQ(zipper.add("data/somefolder/"), true);
        zipper.close();

        // Verify entries were added successfully
        zipper::Unzipper unzipper("ziptest.zip");
        EXPECT_EQ(unzipper.entries().size(), 4u);
        EXPECT_EQ(unzipper.entries()[0].name, "data/somefolder/test.txt");
        EXPECT_EQ(unzipper.entries()[1].name, "data/somefolder/test.txt");
        EXPECT_EQ(unzipper.entries()[2].name, "test.txt");
        EXPECT_EQ(unzipper.entries()[3].name, "test.txt");
        unzipper.close();

        ASSERT_TRUE(helper::removeFileOrDir("ziptest.zip"));
        ASSERT_TRUE(helper::removeFileOrDir("data"));
    }

    // Test without the '/' with and without the option Zipper::SaveHierarchy
    {
        Zipper zipper("ziptest.zip", Zipper::OpenFlags::Overwrite);
        EXPECT_EQ(zipper.add("data/somefolder", Zipper::SaveHierarchy), false);
        std::string error =
            "Cannot open file: 'data/somefolder'"; // FIXME:
                                                   // Path::toNativeSeparators
        EXPECT_STREQ(zipper.error().message().c_str(), error.c_str());
        EXPECT_EQ(zipper.add("data/somefolder", Zipper::SaveHierarchy), false);
        EXPECT_EQ(zipper.add("data/somefolder"), false);
        EXPECT_EQ(zipper.add("data/somefolder"), false);
        zipper.close();

        // Verify entries were added successfully
        zipper::Unzipper unzipper("ziptest.zip");
        EXPECT_EQ(unzipper.entries().size(), 0u);
        unzipper.close();

        ASSERT_TRUE(helper::removeFileOrDir("ziptest.zip"));
        ASSERT_TRUE(helper::removeFileOrDir("data"));
    }
}

//=============================================================================
// Tests for issue #33: Security vulnerability - Zip Slip - Zipping
// https://github.com/sebastiandev/zipper/issues/33
//=============================================================================
TEST(ZipTests, Issue33_zipping)
{
    // Test with path traversal attack (using "../") is not allowed.
    {
        Path::remove("ziptest.zip");
        Zipper zipper("ziptest.zip", Zipper::OpenFlags::Overwrite);
        EXPECT_FALSE(
            helper::zipAddFile(zipper, "Test1.txt", "hello", "../Test1"));
        EXPECT_STREQ(
            zipper.error().message().c_str(),
            "Security error: forbidden insertion of '../Test1' "
            "(canonical: '../Test1') to prevent possible Zip Slip attack");
        zipper.close();

        // Verify no entries were not added.
        Unzipper unzipper("ziptest.zip");
        EXPECT_EQ(unzipper.entries().size(), 0u);
        unzipper.close();

        ASSERT_TRUE(helper::removeFileOrDir("ziptest.zip"));
    }

    // Test with normalized path (foo/../Test1 -> Test1) is allowed.
    {
        Zipper zipper("ziptest.zip", Zipper::OpenFlags::Overwrite);
        EXPECT_TRUE(
            helper::zipAddFile(zipper, "Test1.txt", "world", "foo/../Test1"));
        zipper.close();

        // Verify entries were added successfully.
        Unzipper unzipper("ziptest.zip");
        EXPECT_EQ(unzipper.entries().size(), 1u);
        EXPECT_STREQ(unzipper.entries()[0].name.c_str(), "Test1");
        unzipper.close();

        ASSERT_TRUE(helper::removeFileOrDir("ziptest.zip"));
    }
}

//=============================================================================
// Tests for issue #33: Security vulnerability - Zip Slip - Unzipping
// https://github.com/sebastiandev/zipper/issues/33
//=============================================================================
TEST(ZipTests, Issue33_unzipping)
{
    // Test with path traversal attack when extracting. Check that the file is
    // not created.
    {
        ASSERT_FALSE(Path::exist("../Test1"));

        Unzipper unzipper(PWD "/issues/issue33_1.zip");
        EXPECT_EQ(unzipper.entries().size(), 1u);
        EXPECT_STREQ(unzipper.entries()[0].name.c_str(), "../Test1");
        EXPECT_EQ(unzipper.extract("../Test1"), false);
#if defined(_WIN32)
        EXPECT_STREQ(unzipper.error().message().c_str(),
                     "Security error: entry '..\\Test1' would be outside your "
                     "target directory");
#else
        EXPECT_STREQ(unzipper.error().message().c_str(),
                     "Security error: entry '../Test1' would be outside your "
                     "target directory");
#endif

        unzipper.close();
        EXPECT_FALSE(Path::exist("../Test1"));
    }

    // Test with normalized path within zip (foo/../Test1 -> Test1). Check
    // that the file is created.
    {
        ASSERT_TRUE(helper::removeFileOrDir("Test1"));

        Unzipper unzipper(PWD "/issues/issue33_2.zip");
        EXPECT_EQ(unzipper.entries().size(), 1u);
        EXPECT_STREQ(unzipper.entries()[0].name.c_str(), "foo/../Test1");
        EXPECT_TRUE(unzipper.extract("foo/../Test1"));
        unzipper.close();
        EXPECT_TRUE(Path::exist("Test1"));
        EXPECT_TRUE(Path::isFile("Test1"));
        EXPECT_STREQ(helper::readFileContent("Test1").c_str(), "hello");

        ASSERT_TRUE(helper::removeFileOrDir("Test1"));
    }
}

//=============================================================================
// Tests for issue #34: Empty directories in zip archives
// https://github.com/sebastiandev/zipper/issues/34
//=============================================================================
TEST(ZipTests, Issue34)
{
    zipper::Unzipper unzipper(PWD "/issues/issue34.zip");
    EXPECT_EQ(unzipper.entries().size(), 13u);
    EXPECT_STREQ(unzipper.entries()[0].name.c_str(), "issue34/");
    EXPECT_STREQ(unzipper.entries()[1].name.c_str(), "issue34/1/");
    EXPECT_STREQ(unzipper.entries()[2].name.c_str(), "issue34/1/.dummy");
    EXPECT_STREQ(unzipper.entries()[3].name.c_str(), "issue34/1/2/");
    EXPECT_STREQ(unzipper.entries()[4].name.c_str(), "issue34/1/2/3/");
    EXPECT_STREQ(unzipper.entries()[5].name.c_str(), "issue34/1/2/3/4/");
    EXPECT_STREQ(unzipper.entries()[6].name.c_str(), "issue34/1/2/3_1/");
    EXPECT_STREQ(unzipper.entries()[7].name.c_str(), "issue34/1/2/3_1/3.1.txt");
    EXPECT_STREQ(unzipper.entries()[8].name.c_str(), "issue34/1/2/foobar.txt");
    EXPECT_STREQ(unzipper.entries()[9].name.c_str(), "issue34/11/");
    EXPECT_STREQ(unzipper.entries()[10].name.c_str(), "issue34/11/foo/");
    EXPECT_STREQ(unzipper.entries()[11].name.c_str(), "issue34/11/foo/bar/");
    EXPECT_STREQ(unzipper.entries()[12].name.c_str(),
                 "issue34/11/foo/bar/here.txt");

    // Test extracting to temp directory
    std::string temp_dir = Path::getTempDirectory();
    ASSERT_TRUE(helper::removeFileOrDir(temp_dir + "issue34"));
    EXPECT_TRUE(unzipper.extractAll(temp_dir));

    // Verify all directories and files were correctly extracted
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/.dummy"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/2/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/2/3/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/2/3/4/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/2/3_1/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/2/3_1/3.1.txt"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/1/2/foobar.txt"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/11/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/11/foo/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/11/foo/bar/"));
    EXPECT_TRUE(Path::exist(temp_dir + "issue34/11/foo/bar/here.txt"));

    // Verify directories and files have the correct type
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/1/"));
    EXPECT_TRUE(Path::isFile(temp_dir + "issue34/1/.dummy"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/1/2/"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/1/2/3/"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/1/2/3/4/"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/1/2/3_1/"));
    EXPECT_TRUE(Path::isFile(temp_dir + "issue34/1/2/3_1/3.1.txt"));
    EXPECT_TRUE(Path::isFile(temp_dir + "issue34/1/2/foobar.txt"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/11/"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/11/foo/"));
    EXPECT_TRUE(Path::isDir(temp_dir + "issue34/11/foo/bar/"));
    EXPECT_TRUE(Path::isFile(temp_dir + "issue34/11/foo/bar/here.txt"));

    // Verify file contents
    std::string f1 = temp_dir + "issue34/1/2/3_1/3.1.txt";
    EXPECT_STREQ(helper::readFileContent(f1.c_str()).c_str(), "3.1\n");
    std::string f2 = temp_dir + "issue34/1/2/foobar.txt";
    EXPECT_STREQ(helper::readFileContent(f2.c_str()).c_str(), "foobar.txt\n");
    std::string f3 = temp_dir + "issue34/11/foo/bar/here.txt";
    EXPECT_STREQ(helper::readFileContent(f3.c_str()).c_str(), "");
}

#if !defined(_WIN32)
//=============================================================================
// Tests for issue #83: Error handling for non-existent paths
// https://github.com/sebastiandev/zipper/issues/83
//=============================================================================
TEST(MemoryZipTests, Issue83)
{
    std::string error;

    // Create folder with test file
    Path::remove("data");
    Path::createDir("data/somefolder/");
    ASSERT_TRUE(helper::createFile("data/somefolder/test.txt",
                                   "test file2 compression"));

    // Create zip file
    Path::remove("ziptest.zip");
    Zipper zipper("ziptest.zip", Zipper::OpenFlags::Overwrite);
    EXPECT_TRUE(zipper.add("data/somefolder/", Zipper::SaveHierarchy));
    zipper.close();

    // Test extraction to various invalid paths
    zipper::Unzipper unzipper("ziptest.zip");

    std::string does_not_exist = "/does/not/exist";
    std::string no_permissions = "/usr/bin";

    // Test extraction to non-existent path
    EXPECT_FALSE(unzipper.extract("data/somefolder/test.txt", does_not_exist));
    error = "Cannot create the folder '" +
            Path::toNativeSeparators(does_not_exist + "/data/somefolder") +
            "'. Reason: Permission denied";
    EXPECT_STREQ(unzipper.error().message().c_str(), error.c_str());

    // Test extractAll to non-existent path
    EXPECT_FALSE(unzipper.extractAll(does_not_exist));
    EXPECT_STREQ(unzipper.error().message().c_str(), error.c_str());

    // Test extraction to system path without permissions
    EXPECT_FALSE(unzipper.extract("data/somefolder/test.txt", no_permissions));
    error = "Cannot create the folder '" +
            Path::toNativeSeparators(no_permissions + "/data/somefolder") +
            "'. Reason: Permission denied";
    EXPECT_STREQ(unzipper.error().message().c_str(), error.c_str());

    // Test extractAll to system path without permissions
    EXPECT_FALSE(unzipper.extractAll(no_permissions));
    EXPECT_STREQ(unzipper.error().message().c_str(), error.c_str());

    // Clean up
    ASSERT_TRUE(helper::removeFileOrDir("data"));
    ASSERT_TRUE(helper::removeFileOrDir("ziptest.zip"));
}
#endif

//=============================================================================
// Tests for issue #118: Empty zip file handling
// https://github.com/sebastiandev/zipper/issues/118
//=============================================================================
TEST(MemoryZipTests, Issue118)
{
    ASSERT_TRUE(helper::removeFileOrDir("ziptest.zip"));

    // Create a dummy zip file
    {
        Zipper zipper("ziptest.zip", Zipper::OpenFlags::Overwrite);
        zipper.close();
    }

    // Check there is no entries in empty zip and that extraction fails
    {
        zipper::Unzipper unzipper("ziptest.zip");
        EXPECT_EQ(unzipper.entries().size(), 0u);

        ASSERT_FALSE(
            unzipper.extractAll(Unzipper::OverwriteMode::DoNotOverwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Failed to go to first file"));
        ASSERT_FALSE(unzipper.extractAll(Unzipper::OverwriteMode::Overwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Failed to go to first file"));
        ASSERT_FALSE(
            unzipper.extract("test1.txt", Unzipper::OverwriteMode::Overwrite));
        ASSERT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Invalid info entry"));
        unzipper.close();
    }

    // Add file to the previously empty zip using Append mode
    {
        Zipper zipper("ziptest.zip", Zipper::OpenFlags::Append);
        EXPECT_TRUE(helper::zipAddFile(
            zipper, "test1.txt", "test1 file compression", "test1.txt"));
        zipper.close();
    }

    // Verify entry was added successfully
    {
        zipper::Unzipper unzipper("ziptest.zip");
        std::vector<zipper::ZipEntry> entries = unzipper.entries();
        unzipper.close();
        EXPECT_EQ(entries.size(), 1u);
        EXPECT_STREQ(entries[0].name.c_str(), "test1.txt");
    }

    // Add file to the previously empty zip using Append mode
    {
        Zipper zipper("ziptest.zip", Zipper::OpenFlags::Append);
        EXPECT_TRUE(helper::zipAddFile(
            zipper, "test2.txt", "test2 file compression", "test2.txt"));
        zipper.close();
    }

    // Verify entry was added successfully
    {
        zipper::Unzipper unzipper("ziptest.zip");
        std::vector<zipper::ZipEntry> entries = unzipper.entries();
        unzipper.close();
        EXPECT_EQ(entries.size(), 2u);
        EXPECT_STREQ(entries[0].name.c_str(), "test1.txt");
        EXPECT_STREQ(entries[1].name.c_str(), "test2.txt");
    }

    ASSERT_TRUE(helper::removeFileOrDir("ziptest.zip"));
}

//=============================================================================
// Tests for issue #1: Unicode file names
//=============================================================================
TEST(ZipTests, Issue1_01)
{
    const std::string zipFilename(PWD "/issues/unicode2.zip");
    const std::string file1 = "😊";

    std::cout << "zipFilename: " << zipFilename << std::endl;

    Unzipper unzipper(zipFilename);
    EXPECT_EQ(unzipper.entries().size(), 1u);
    EXPECT_STREQ(unzipper.entries()[0].name.c_str(), file1.c_str());

    ASSERT_TRUE(unzipper.extract(file1));
    ASSERT_TRUE(helper::checkFileExists(file1, "Hello World!"));

    unzipper.close();
    ASSERT_TRUE(helper::removeFileOrDir(file1));
}

#if 0
//=============================================================================
// Tests for issue #1: Unicode file names
//=============================================================================
TEST(ZipTests, Issue1_02)
{
    const std::string zipFilename(PWD "/issues/unicode.zip");
    const std::string file1 = "😊";

    Unzipper unzipper(zipFilename);
    EXPECT_EQ(unzipper.entries().size(), 1u);
    EXPECT_STREQ(unzipper.entries()[0].name.c_str(), file1.c_str());

    ASSERT_TRUE(unzipper.extract(file1));
    ASSERT_TRUE(helper::checkFileExists(file1, "Hello World!"));

    unzipper.close();
    ASSERT_TRUE(helper::removeFileOrDir(file1));
}
#endif
