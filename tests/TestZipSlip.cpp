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