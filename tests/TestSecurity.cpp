#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wundef"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#pragma GCC diagnostic pop

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
//=============================================================================
TEST(ZipSlipTests, LinuxZipSlip)
{
#ifdef _WIN32
    zipper::Unzipper unzipper("issues/zip-slip-win.zip");
#else
    zipper::Unzipper unzipper("issues/zip-slip-linux.zip");
#endif

    std::string temp_dir = "zip-slip-test/";
    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));

    ASSERT_FALSE(unzipper.extractAll(temp_dir));
    ASSERT_THAT(unzipper.error().message(),
                testing::HasSubstr("Security error"));
    unzipper.close();

#ifdef _WIN32
    ASSERT_TRUE(helper::checkFileExists("zip-slip-test/good.txt",
                                        "this is a good one\r\n"));
#else
    ASSERT_TRUE(helper::checkFileExists("zip-slip-test/good.txt",
                                        "this is a good one\n"));
#endif

    ASSERT_TRUE(helper::checkFileDoesNotExist("zip-slip-test/evil.txt"));
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
    ASSERT_EQ(unzipper.getTotalUncompressedSize(), 5461307620); // 5.46 GB
    unzipper.close();

    ASSERT_TRUE(helper::removeFileOrDir(temp_dir));
}