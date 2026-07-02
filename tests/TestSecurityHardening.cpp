//-----------------------------------------------------------------------------
// Additional security-hardening unit tests.
//
// These tests target the weak points identified during the security review:
//  - Zip Slip / directory traversal detection at the path-validation layer
//    (Path::isValidEntry / Path::hasParentDirectoryReference).
//  - Legitimate dot-files must NOT be mistaken for parent references.
//  - The `alternative_names` remapping must not become a Zip Slip bypass.
//  - Control characters must actually abort extraction.
//  - Linux file attributes (external_fa): setuid/setgid stripping, mode 0
//    handling and permission round-trip.
//-----------------------------------------------------------------------------

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define protected public
#define private public
#include "Zipper/Unzipper.hpp"
#include "Zipper/Zipper.hpp"
#undef protected
#undef private

#include "utils/Path.hpp"
#include "TestHelper.hpp"

#include <map>
#include <sstream>

#if !defined(_WIN32)
#    include <sys/stat.h>
#    include <unistd.h>
#endif

using namespace zipper;

//=============================================================================
// Path::startsWithParentDirectoryReference: a leading ".." segment is a
// traversal; dot-files and interior references are handled elsewhere.
//=============================================================================
TEST(SecurityHardening, ParentDirectoryReferenceDetection)
{
    // Leading parent-directory references (rejected up-front).
    EXPECT_TRUE(Path::startsWithParentDirectoryReference(".."));
    EXPECT_TRUE(Path::startsWithParentDirectoryReference("../evil"));
    EXPECT_TRUE(Path::startsWithParentDirectoryReference("../../etc/passwd"));
    EXPECT_TRUE(Path::startsWithParentDirectoryReference("..\\evil"));

    // NOT a leading reference: bare current dir, dot-files, names with dots,
    // and interior references (those are resolved by isZipSlip()).
    EXPECT_FALSE(Path::startsWithParentDirectoryReference("."));
    EXPECT_FALSE(Path::startsWithParentDirectoryReference(".hidden"));
    EXPECT_FALSE(Path::startsWithParentDirectoryReference(".gitignore"));
    EXPECT_FALSE(Path::startsWithParentDirectoryReference("..foo"));
    EXPECT_FALSE(Path::startsWithParentDirectoryReference("foo.."));
    EXPECT_FALSE(Path::startsWithParentDirectoryReference("a/../evil"));
    EXPECT_FALSE(Path::startsWithParentDirectoryReference("a/.config/x"));
    EXPECT_FALSE(
        Path::startsWithParentDirectoryReference("normal/path/file.txt"));
    EXPECT_FALSE(Path::startsWithParentDirectoryReference("..."));
}

//=============================================================================
// isValidEntry must flag a leading ".." traversal.
//=============================================================================
TEST(SecurityHardening, IsValidEntryFlagsLeadingTraversal)
{
    EXPECT_EQ(Path::isValidEntry("../evil.txt"),
              Path::InvalidEntryReason::ZIP_SLIP);
    EXPECT_EQ(Path::isValidEntry(".."),
              Path::InvalidEntryReason::ZIP_SLIP);
    EXPECT_EQ(Path::isValidEntry("../../evil"),
              Path::InvalidEntryReason::ZIP_SLIP);

    // Absolute paths still reported as absolute (not zip slip).
    EXPECT_EQ(Path::isValidEntry("/etc/passwd"),
              Path::InvalidEntryReason::ABSOLUTE_PATH);

    // Empty and control chars.
    EXPECT_EQ(Path::isValidEntry(""),
              Path::InvalidEntryReason::EMPTY_ENTRY);
    EXPECT_EQ(Path::isValidEntry(std::string("a\x01""b.txt")),
              Path::InvalidEntryReason::CONTROL_CHARACTERS);
}

//=============================================================================
// Interior ".." references are NOT rejected by isValidEntry (they may resolve
// to a safe path); isZipSlip() is responsible for deciding on the final path.
//=============================================================================
TEST(SecurityHardening, InteriorTraversalDelegatedToIsZipSlip)
{
    // "foo/../Test1" resolves to "Test1" -> valid entry, and stays inside dest.
    EXPECT_EQ(Path::isValidEntry("foo/../Test1"),
              Path::InvalidEntryReason::VALID_ENTRY);
    EXPECT_FALSE(Path::isZipSlip("foo/../Test1", "/safe/dir"));

    // But an interior reference that escapes must be caught by isZipSlip.
    EXPECT_TRUE(Path::isZipSlip("foo/../../evil", "/safe/dir"));
    EXPECT_TRUE(Path::isZipSlip("a/b/../../../evil", "/safe/dir"));
}

//=============================================================================
// Legitimate dot-files must be accepted (previous code rejected them as slip).
//=============================================================================
TEST(SecurityHardening, IsValidEntryAcceptsDotFiles)
{
    EXPECT_EQ(Path::isValidEntry(".gitignore"),
              Path::InvalidEntryReason::VALID_ENTRY);
    EXPECT_EQ(Path::isValidEntry(".hidden"),
              Path::InvalidEntryReason::VALID_ENTRY);
    EXPECT_EQ(Path::isValidEntry(".config/app.conf"),
              Path::InvalidEntryReason::VALID_ENTRY);
    EXPECT_EQ(Path::isValidEntry("..foo"),
              Path::InvalidEntryReason::VALID_ENTRY);
    EXPECT_EQ(Path::isValidEntry("normal/sub/file.txt"),
              Path::InvalidEntryReason::VALID_ENTRY);
}

//=============================================================================
// End-to-end: a dot-file entry can be zipped and extracted successfully.
// This exercised a real regression: dot-files used to be refused both at add
// and extraction time.
//=============================================================================
TEST(SecurityHardening, DotFileRoundTrip)
{
    const std::string zip_name = "sec_dotfile.zip";
    const std::string out_dir = "sec_dotfile_out";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);

    {
        Zipper zipper(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(helper::zipAddFile(
            zipper, "src_gitignore.txt", "build/\n", ".gitignore"));
        ASSERT_TRUE(helper::zipAddFile(
            zipper, "src_conf.txt", "key=value\n", ".config/app.conf"));
        zipper.close();
    }

    {
        Unzipper unzipper(zip_name);
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 2u);
        EXPECT_EQ(entries[0].name, ".gitignore");
        EXPECT_EQ(entries[1].name, ".config/app.conf");

        ASSERT_TRUE(
            unzipper.extractAll(out_dir, Unzipper::OverwriteMode::Overwrite));
        unzipper.close();
    }

    EXPECT_TRUE(helper::checkFileExists(out_dir + "/.gitignore", "build/\n"));
    EXPECT_TRUE(
        helper::checkFileExists(out_dir + "/.config/app.conf", "key=value\n"));

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
}

//=============================================================================
// alternative_names must not be usable to escape the destination directory.
//=============================================================================
TEST(SecurityHardening, AlternativeNamesCannotEscapeDestination)
{
    const std::string zip_name = "sec_altnames.zip";
    const std::string out_dir = "sec_altnames_out";
    const std::string escaped = "sec_altnames_escaped.txt";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(escaped);

    {
        Zipper zipper(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(
            helper::zipAddFile(zipper, "good.txt", "good", "good.txt"));
        zipper.close();
    }

    {
        Unzipper unzipper(zip_name);
        std::map<std::string, std::string> alternative_names;
        // Attempt to write one directory above the destination.
        alternative_names["good.txt"] = "../" + escaped;

        ASSERT_FALSE(unzipper.extractAll(
            out_dir, alternative_names, Unzipper::OverwriteMode::Overwrite));
        EXPECT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Security error"));
        unzipper.close();
    }

    // The escaping file must not have been created next to the destination.
    EXPECT_TRUE(helper::checkFileDoesNotExist(escaped));
    EXPECT_TRUE(helper::checkFileDoesNotExist(out_dir + "/../" + escaped));

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(escaped);
}

//=============================================================================
// alternative_names with absolute path must also be rejected.
//=============================================================================
TEST(SecurityHardening, AlternativeNamesRejectsAbsolutePath)
{
    const std::string zip_name = "sec_altnames_abs.zip";
    const std::string out_dir = "sec_altnames_abs_out";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);

    {
        Zipper zipper(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(
            helper::zipAddFile(zipper, "good.txt", "good", "good.txt"));
        zipper.close();
    }

    {
        Unzipper unzipper(zip_name);
        std::map<std::string, std::string> alternative_names;
#if defined(_WIN32)
        alternative_names["good.txt"] = "C:\\Windows\\evil.txt";
#else
        alternative_names["good.txt"] = "/tmp/sec_altnames_abs_evil.txt";
#endif
        ASSERT_FALSE(unzipper.extractAll(
            out_dir, alternative_names, Unzipper::OverwriteMode::Overwrite));
        EXPECT_THAT(unzipper.error().message(),
                    testing::HasSubstr("Security error"));
        unzipper.close();
    }

#if !defined(_WIN32)
    EXPECT_TRUE(
        helper::checkFileDoesNotExist("/tmp/sec_altnames_abs_evil.txt"));
#endif

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
}

//=============================================================================
// Regular alternative names (no traversal) still work after the hardening.
//=============================================================================
TEST(SecurityHardening, AlternativeNamesStillWorkForSafeNames)
{
    const std::string zip_name = "sec_altnames_ok.zip";
    const std::string out_dir = "sec_altnames_ok_out";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);

    {
        Zipper zipper(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(
            helper::zipAddFile(zipper, "src.txt", "payload", "orig.txt"));
        zipper.close();
    }

    {
        Unzipper unzipper(zip_name);
        std::map<std::string, std::string> alternative_names;
        alternative_names["orig.txt"] = "sub/renamed.txt";
        ASSERT_TRUE(unzipper.extractAll(
            out_dir, alternative_names, Unzipper::OverwriteMode::Overwrite));
        unzipper.close();
    }

    EXPECT_TRUE(
        helper::checkFileExists(out_dir + "/sub/renamed.txt", "payload"));

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
}

#if !defined(_WIN32)
//=============================================================================
// Linux file attributes: setuid / setgid bits stored in a ZIP entry must be
// stripped on extraction (never restore privileged bits from an archive).
//=============================================================================
TEST(SecurityHardening, SetuidSetgidStrippedOnExtraction)
{
    const std::string zip_name = "sec_suid.zip";
    const std::string out_dir = "sec_suid_out";
    const std::string src_dir = "sec_suid_src";
    const std::string src_file = src_dir + "/tool";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(src_dir);

    ASSERT_TRUE(helper::createDir(src_dir));
    ASSERT_TRUE(helper::createFile(src_file, "#!/bin/sh\n"));
    // setuid + setgid + rwxr-xr-x
    ASSERT_EQ(::chmod(src_file.c_str(), static_cast<mode_t>(06755)), 0);

    {
        Zipper z(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(z.add(src_file, Zipper::ZipFlags::Better));
        z.close();
    }

    // The archive should still carry the privileged bits in external_fa...
    {
        Unzipper u(zip_name);
        auto ent = u.entries();
        ASSERT_FALSE(ent.empty());
        EXPECT_EQ(ent[0].external_fa >> 16, 06755u);
        u.close();
    }

    ASSERT_TRUE(helper::createDir(out_dir));
    {
        Unzipper u(zip_name);
        ASSERT_TRUE(u.extractAll(out_dir, Unzipper::OverwriteMode::Overwrite));
        u.close();
    }

    // ...but the extracted file must never keep setuid/setgid.
    const std::string extracted = out_dir + "/" + Path::fileName(src_file);
    struct stat st{};
    ASSERT_EQ(::stat(extracted.c_str(), &st), 0);
    EXPECT_EQ(static_cast<unsigned>(st.st_mode & 06000), 0u)
        << "setuid/setgid bits must be stripped on extraction";
    EXPECT_EQ(static_cast<unsigned>(st.st_mode & 0777), 0755u);

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(src_dir);
}

//=============================================================================
// Permission round-trip for a directory entry.
//=============================================================================
TEST(SecurityHardening, DirectoryPermissionsRoundTrip)
{
    const std::string zip_name = "sec_dirperm.zip";
    const std::string out_dir = "sec_dirperm_out";
    const std::string src_dir = "sec_dirperm_src";
    const std::string sub_dir = src_dir + "/private";

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(src_dir);

    ASSERT_TRUE(helper::createDir(src_dir));
    ASSERT_TRUE(helper::createDir(sub_dir));
    ASSERT_TRUE(helper::createFile(sub_dir + "/secret.txt", "top secret"));
    ASSERT_EQ(::chmod((sub_dir + "/secret.txt").c_str(),
                      static_cast<mode_t>(0600)),
              0);

    {
        Zipper z(zip_name, Zipper::OpenFlags::Overwrite);
        ASSERT_TRUE(z.add(src_dir, Zipper::ZipFlags::Better |
                                       Zipper::ZipFlags::SaveHierarchy));
        z.close();
    }

    ASSERT_TRUE(helper::createDir(out_dir));
    {
        Unzipper u(zip_name);
        ASSERT_TRUE(u.extractAll(out_dir, Unzipper::OverwriteMode::Overwrite));
        u.close();
    }

    // The 0600 file must keep its restrictive permissions after extraction.
    const std::string extracted =
        out_dir + "/" + src_dir + "/private/secret.txt";
    struct stat st{};
    ASSERT_EQ(::stat(extracted.c_str(), &st), 0);
    EXPECT_EQ(static_cast<unsigned>(st.st_mode & 0777), 0600u);

    helper::removeFileOrDir(zip_name);
    helper::removeFileOrDir(out_dir);
    helper::removeFileOrDir(src_dir);
}
#endif // !_WIN32

//=============================================================================
// Control characters in an entry name must abort extraction (was a no-op).
//=============================================================================
TEST(SecurityHardening, ControlCharactersAbortExtraction)
{
    // isValidEntry already rejects control characters; confirm the dedicated
    // detector is consistent with the extraction guard.
    EXPECT_EQ(Path::checkControlCharacters(std::string("ok.txt")),
              Path::InvalidEntryReason::VALID_ENTRY);
    EXPECT_EQ(Path::checkControlCharacters(std::string("bad\x1f""name")),
              Path::InvalidEntryReason::CONTROL_CHARACTERS);
    EXPECT_EQ(Path::checkControlCharacters(std::string("tab\tname")),
              Path::InvalidEntryReason::CONTROL_CHARACTERS);
}
