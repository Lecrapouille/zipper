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
// Tests for zip-slip vulnerability
// From https://github.com/snyk/zip-slip-vulnerability/tree/master
//=============================================================================
TEST(ZipSlipTests, ZipSlip)
{
#if defined(_WIN32)
    std::string zip_path(PWD "/issues/zip-slip-win.zip");
#else
    std::string zip_path(PWD "/issues/zip-slip-linux.zip");
#endif
    std::string temp_dir = "zip-slip-test/";

    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));

    Unzipper unzipper(zip_path);
    ASSERT_FALSE(unzipper.extractAll(temp_dir));
    ASSERT_THAT(unzipper.error().message(),
                testing::HasSubstr("Security error"));
    unzipper.close();

    // Check that the good file is present.
    // Check that the evil file is not present.
    ASSERT_TRUE(helper::checkFileExists("zip-slip-test/good.txt",
                                        "this is a good one\n"));
    ASSERT_TRUE(helper::checkFileDoesNotExist("zip-slip-test/evil.txt"));

    // Clean up
    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));
}

//=============================================================================
// https://github.com/Lecrapouille/zipper/issues/7
// https://unforgettable.dk/
//=============================================================================
TEST(ZipSlipTests, Forty42)
{
    std::string zip_path(PWD "/issues/42.zip");
    std::string temp_dir = "42/";
    std::string password = "42";
    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));

    Unzipper unzipper(zip_path, password);
    auto entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 16);
    ASSERT_EQ(entries[0].name, "lib 0.zip");
    ASSERT_EQ(entries[1].name, "lib 1.zip");
    ASSERT_EQ(entries[2].name, "lib 2.zip");
    ASSERT_EQ(entries[3].name, "lib 3.zip");
    ASSERT_EQ(entries[4].name, "lib 4.zip");
    ASSERT_EQ(entries[5].name, "lib 5.zip");
    ASSERT_EQ(entries[6].name, "lib 6.zip");
    ASSERT_EQ(entries[7].name, "lib 7.zip");
    ASSERT_EQ(entries[8].name, "lib 8.zip");
    ASSERT_EQ(entries[9].name, "lib 9.zip");
    ASSERT_EQ(entries[10].name, "lib a.zip");
    ASSERT_EQ(entries[11].name, "lib b.zip");
    ASSERT_EQ(entries[12].name, "lib c.zip");
    ASSERT_EQ(entries[13].name, "lib d.zip");
    ASSERT_EQ(entries[14].name, "lib e.zip");
    ASSERT_EQ(entries[15].name, "lib f.zip");
    ASSERT_TRUE(unzipper.extractAll(temp_dir));
    unzipper.close();

    ASSERT_TRUE(helper::checkFileExists("42/lib 0.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 1.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 2.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 3.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 4.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 5.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 6.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 7.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 8.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib 9.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib a.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib b.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib c.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib d.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib e.zip"));
    ASSERT_TRUE(helper::checkFileExists("42/lib f.zip"));

    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));
}

//=============================================================================
// https://github.com/Lecrapouille/zipper/issues/7
// https://www.bamsoftware.com/hacks/zipbomb/
//=============================================================================
TEST(ZipSlipTests, ZipBomb)
{
    std::string zip_path(PWD "/issues/zbsm.zip");
    std::string temp_dir = "zip-bomb/";
    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));

    Unzipper unzipper(zip_path);
    auto entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 250);
    for (size_t i = 0; i < entries.size(); ++i)
    {
        ASSERT_EQ(entries[i].name, helper::intToBase36(i));
    }
    ASSERT_EQ(unzipper.sizeOnDisk(), 5461307620); // 5.46 GB
    unzipper.close();

    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));
}

#if 0
//=============================================================================
// This is not a test but just an helper to create a nasty zip file. You should
// deactivate the security before. And
//=============================================================================
TEST(ZipSlipTests, CreateNastyZipFile)
{
    std::vector<std::string> forbidden_entries = {
        "corr<.txt", "corr>.txt", "corr:.txt", "corr\".txt",
        "corr|.txt", "corr*.txt", "corr?.txt",
    };

    std::string zip_path(PWD "/issues/nasty.zip");
    Zipper zipper(zip_path, Zipper::OpenFlags::Overwrite);

    // Forbidden characters
    for (const auto& forbidden_entry : forbidden_entries)
    {
        ASSERT_TRUE(helper::zipAddFile(
            zipper, "corrupted.txt", "corrupted", forbidden_entry.c_str()));
    }
    // Control characters
    ASSERT_TRUE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "\x00corrupted.txt"));

    // Absolute path: allowed: the leading slash is removed
    ASSERT_TRUE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "/foo/bar/corrupted1.txt"));

    // Zip slip: allowed: the leading slash is removed
    ASSERT_TRUE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "/../corrupted2.txt"));

    // Zip slip: not allowed: the leading slash is not removed
    ASSERT_TRUE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "../corrupted3.txt"));

    zipper.close();

    // Check contents
    size_t i = 0;
    Unzipper unzipper(zip_path);
    ASSERT_EQ(unzipper.entries().size(), forbidden_entries.size() + 4u);
    for (i = 0; i < forbidden_entries.size(); ++i)
    {
        ASSERT_EQ(unzipper.entries()[i].name, forbidden_entries[i]);
    }
    ASSERT_STREQ(unzipper.entries()[i].name.c_str(), "\x00corrupted.txt");
    ASSERT_STREQ(unzipper.entries()[i + 1].name.c_str(),
                 "/foo/bar/corrupted1.txt");
    ASSERT_STREQ(unzipper.entries()[i + 2].name.c_str(), "/../corrupted2.txt");
    ASSERT_STREQ(unzipper.entries()[i + 3].name.c_str(), "../corrupted3.txt");
    unzipper.close();
}
#endif

//=============================================================================
// Try add corrupted file to zip
//=============================================================================
TEST(ZipSlipTests, CorruptedFile)
{
    std::vector<std::string> forbidden_entries = {
        "corr<.txt", "corr>.txt", "corr:.txt", "corr\".txt",
        "corr|.txt", "corr*.txt", "corr?.txt",
    };

    Zipper zipper("corrupted.zip", Zipper::OpenFlags::Overwrite);

    // Forbidden characters
    for (const auto& forbidden_entry : forbidden_entries)
    {
        ASSERT_FALSE(helper::zipAddFile(
            zipper, "corrupted.txt", "corrupted", forbidden_entry.c_str()));
        ASSERT_THAT(zipper.error().message(),
                    testing::HasSubstr("contains forbidden characters"));
    }

    // Control characters
    ASSERT_FALSE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "\x00corrupted.txt"));
    ASSERT_THAT(zipper.error().message(),
                testing::HasSubstr("contains control characters"));

    // Absolute path: allowed: the leading slash is removed
    ASSERT_TRUE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "/foo/bar/corrupted1.txt"));

    // Zip slip: allowed: the leading slash is removed
    ASSERT_TRUE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "/../corrupted2.txt"));

    // Zip slip: not allowed: the leading slash is not removed
    ASSERT_FALSE(helper::zipAddFile(
        zipper, "corrupted.txt", "corrupted", "../corrupted3.txt"));
    ASSERT_THAT(zipper.error().message(),
                testing::HasSubstr(
                    "could be used to escape the destination directory"));
    zipper.close();

    // Check contents
    Unzipper unzipper("corrupted.zip");
    ASSERT_EQ(unzipper.entries().size(), 2);
    ASSERT_EQ(unzipper.entries()[0].name, "foo/bar/corrupted1.txt");
    ASSERT_EQ(unzipper.entries()[1].name, "corrupted2.txt");
    unzipper.close();

    ASSERT_TRUE(helper::removeFileOrDir("corrupted.zip"));
}