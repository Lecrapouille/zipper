//-----------------------------------------------------------------------------
// Unit tests: ZIP external_fa (Unix mode bits) round-trip.
//-----------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "Zipper/Unzipper.hpp"
#include "Zipper/Zipper.hpp"
#include "TestHelper.hpp"

#include <sstream>

#if !defined(_WIN32)
#    include <sys/stat.h>
#    include <unistd.h>
#endif

using namespace zipper;

#if !defined(_WIN32)

//=============================================================================
TEST(UnixExternalAttributes, ExecutablePreservedWhenZippingFromDiskPath)
{
    const std::string zip_name = "ziptest_unix_exe.zip";
    const std::string out_dir = "ziptest_unix_exe_out";
    const std::string src_dir = "ziptest_unix_exe_src";
    const std::string src_file = src_dir + "/runner";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(src_dir);

    ASSERT_TRUE(helper::createDir(src_dir));
    ASSERT_TRUE(helper::createFile(src_file, "#!/bin/sh\necho ok\n"));
    ASSERT_EQ(::chmod(src_file.c_str(), static_cast<mode_t>(0755)), 0);

    {
        Zipper z(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(z.add(src_file, Zipper::ZipFlags::Better));
        z.close();
    }

    ASSERT_TRUE(helper::createDir(out_dir));

    {
        Unzipper u(zip_name);
        ASSERT_TRUE(u.extractAll(out_dir, Unzipper::OverwriteMode::Overwrite));
        u.close();
    }

    const std::string extracted = out_dir + "/" + Path::fileName(src_file);

    struct stat st{};
    ASSERT_EQ(::stat(extracted.c_str(), &st), 0);
    ASSERT_TRUE(S_ISREG(st.st_mode));
    EXPECT_EQ(st.st_mode & 07777, 0755u);

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(src_dir);
}

//=============================================================================
TEST(UnixExternalAttributes, EntriesExposeNonZeroExternalFaForUnixPackedFile)
{
    const std::string zip_name = "ziptest_unix_meta.zip";
    const std::string src_dir = "ziptest_unix_meta_src";
    const std::string src_file = src_dir + "/marked";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(src_dir);

    ASSERT_TRUE(helper::createDir(src_dir));
    ASSERT_TRUE(helper::createFile(src_file, "x"));
    ASSERT_EQ(::chmod(src_file.c_str(), static_cast<mode_t>(0640)), 0);

    {
        Zipper z(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(z.add(src_file, Zipper::ZipFlags::Better));
        z.close();
    }

    Unzipper unzipper(zip_name);
    auto ent = unzipper.entries();
    ASSERT_FALSE(ent.empty());

    ZipEntry found;
    bool got = false;
    for (auto const& e : ent)
    {
        if (e.name == Path::fileName(src_file))
        {
            found = e;
            got = true;
            break;
        }
    }
    ASSERT_TRUE(got);
    ASSERT_NE(found.external_fa >> 16, 0u);
    EXPECT_EQ(found.external_fa >> 16, 0640u);

    unzipper.close();
    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(src_dir);
}

#endif // !_WIN32

//=============================================================================
// Stream-based adds leave external_fa at 0 (no host stat); extraction must
// still succeed. On POSIX, apply_zip_unix_permissions is a no-op for that case.
//=============================================================================
TEST(UnixExternalAttributes, SkippedOutsidePosixHosts)
{
    const std::string zip_name = "ziptest_extfa_zero.zip";
    const std::string out_dir = "ziptest_extfa_zero_out";
    const std::string entry_name = "payload.txt";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);

    {
        std::istringstream input("payload");
        Zipper z(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(z.add(input, entry_name, Zipper::ZipFlags::Better));
        z.close();
    }

    {
        Unzipper u(zip_name);
        auto const ent = u.entries();
        ASSERT_FALSE(ent.empty());

        ZipEntry found;
        bool got = false;
        for (auto const& e : ent)
        {
            if (e.name == entry_name)
            {
                found = e;
                got = true;
                break;
            }
        }
        ASSERT_TRUE(got);
        EXPECT_EQ(found.external_fa, 0u);
        u.close();
    }

    ASSERT_TRUE(helper::createDir(out_dir));

    {
        Unzipper u(zip_name);
        ASSERT_TRUE(u.extractAll(out_dir, Unzipper::OverwriteMode::Overwrite));
        u.close();
    }

    const std::string extracted = out_dir + "/" + entry_name;
    EXPECT_TRUE(helper::checkFileExists(extracted, "payload"));

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
}
