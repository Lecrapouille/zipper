#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  include "gmock/gmock.h"
#  include "gtest/gtest.h"
# pragma GCC diagnostic pop

#include "utils/Path.hpp"

#ifndef PWD
#  error "PWD is not defined"
#endif

using namespace zipper;

TEST(TestDir, exist)
{
   ASSERT_EQ(Path::exist(PWD), true);
   ASSERT_EQ(Path::exist(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::exist(""), false);
   ASSERT_EQ(Path::exist("fsdfqfz"), false);
}

TEST(TestDir, isDir)
{
   ASSERT_EQ(Path::isDir(PWD), true);
   ASSERT_EQ(Path::isDir(PWD"/main.cpp"), false);
   ASSERT_EQ(Path::isDir(""), false);
   ASSERT_EQ(Path::isDir("sdsdqsd"), false);
}

TEST(TestDir, isFile)
{
   ASSERT_EQ(Path::isFile(PWD), false);
   ASSERT_EQ(Path::isFile(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::isFile(""), false);
   ASSERT_EQ(Path::isFile("aazaza"), false);
}

TEST(TestDir, isReadable)
{
   ASSERT_EQ(Path::isReadable(PWD), true);
   ASSERT_EQ(Path::isReadable(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::isReadable(""), false);
   ASSERT_EQ(Path::isReadable("qdqdqsdqdq"), false);
#if ! defined(_WIN32)
   ASSERT_EQ(Path::isReadable("/usr/bin"), true);
#endif
}

TEST(TestDir, isWritable)
{
   ASSERT_EQ(Path::isWritable(PWD), true);
   ASSERT_EQ(Path::isWritable(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::isWritable(""), false);
   ASSERT_EQ(Path::isWritable("qdqdqsdqdq"), false);
#if ! defined(_WIN32)
   ASSERT_EQ(Path::isWritable("/usr/bin"), false);
#endif
}

TEST(TestDir, fileName)
{
   ASSERT_STREQ(Path::fileName("/foo/bar/file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("/foo/bar/file.foo.txt").c_str(), "file.foo.txt");
   ASSERT_STREQ(Path::fileName("/foo/bar").c_str(), "bar");
   ASSERT_STREQ(Path::fileName("/foo/bar/").c_str(), "");
   ASSERT_STREQ(Path::fileName("./foo/../bar/file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("./foo/../bar/../file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("").c_str(), "");
   ASSERT_STREQ(Path::fileName("..").c_str(), "..");
   ASSERT_STREQ(Path::fileName("/").c_str(), "");
   ASSERT_STREQ(Path::fileName("//").c_str(), "");
   ASSERT_STREQ(Path::fileName("//.").c_str(), ".");

   ASSERT_STREQ(Path::fileName("/foo/bar.txt").c_str(), "bar.txt");
   ASSERT_STREQ(Path::fileName("/foo/.bar").c_str(), ".bar");
   ASSERT_STREQ(Path::fileName("/foo/bar/").c_str(), "");
   ASSERT_STREQ(Path::fileName("/foo/.").c_str(), ".");
   ASSERT_STREQ(Path::fileName("/foo/..").c_str(), "..");
   ASSERT_STREQ(Path::fileName(".").c_str(), ".");
   ASSERT_STREQ(Path::fileName("..").c_str(), "..");
   ASSERT_STREQ(Path::fileName("/").c_str(), "");
   ASSERT_STREQ(Path::fileName("//host").c_str(), "host");
}

TEST(TestDir, dirName)
{
   ASSERT_STREQ(Path::dirName("/foo/bar/file.txt").c_str(), "/foo/bar");
   ASSERT_STREQ(Path::dirName("/foo/bar/file.foo.txt").c_str(), "/foo/bar");
   ASSERT_STREQ(Path::dirName("/foo/bar").c_str(), "/foo");
   ASSERT_STREQ(Path::dirName("/foo/bar/").c_str(), "/foo/bar");
   ASSERT_STREQ(Path::dirName("./foo/../bar/file.txt").c_str(), "./foo/../bar");
   ASSERT_STREQ(Path::dirName("./foo/../bar/../file.txt").c_str(), "./foo/../bar/..");
   ASSERT_STREQ(Path::dirName("/var/tmp/.").c_str(), "/var/tmp");;
   ASSERT_STREQ(Path::dirName("/usr/lib").c_str(), "/usr");
   ASSERT_STREQ(Path::dirName("/usr/").c_str(), "/usr");
   ASSERT_STREQ(Path::dirName("/usr").c_str(), "/");
   ASSERT_STREQ(Path::dirName("usr").c_str(), "");
   ASSERT_STREQ(Path::dirName("/").c_str(), "/");
   ASSERT_STREQ(Path::dirName(".").c_str(), "");
   ASSERT_STREQ(Path::dirName("..").c_str(), "");
   ASSERT_STREQ(Path::dirName("//").c_str(), "/");
   ASSERT_STREQ(Path::dirName("//.").c_str(), "/");
}

TEST(TestDir, extension)
{
   ASSERT_STREQ(Path::extension("/foo/bar/file.txt").c_str(), "txt");
   ASSERT_STREQ(Path::extension("/foo/bar/file.foo.txt").c_str(), "foo.txt");
   ASSERT_STREQ(Path::extension(".txt").c_str(), "txt");
   ASSERT_STREQ(Path::extension("/a/b.c/d").c_str(), "");
   ASSERT_STREQ(Path::extension("").c_str(), "");
   ASSERT_STREQ(Path::extension("txt").c_str(), "");
   ASSERT_STREQ(Path::extension("foo.bar.baz").c_str(), "bar.baz");
   ASSERT_STREQ(Path::extension(".bar.baz.txt").c_str(), "bar.baz.txt");
   ASSERT_STREQ(Path::extension(".").c_str(), "");
}

TEST(TestDir, createDir)
{
   std::string const c_fooBar(Path::getTempDirectory() + "/foo/bar");

   ASSERT_EQ(Path::exist(Path::getTempDirectory()), true);
   std::remove(c_fooBar.c_str());

   ASSERT_EQ(Path::createDir("foo/bar", Path::getTempDirectory()), true);
   ASSERT_EQ(Path::exist(c_fooBar), true);
   ASSERT_EQ(Path::isDir(c_fooBar), true);
   ASSERT_EQ(Path::isWritable(c_fooBar), true);
   ASSERT_EQ(Path::isReadable(c_fooBar), true);

   ASSERT_EQ(Path::createDir("foo/bar", "doesnotexist"), false);
   ASSERT_EQ(Path::exist("doesnotexist/foo/bar"), false);

   ASSERT_EQ(Path::createDir("foo", "/usr/bin"), false);
   ASSERT_EQ(Path::exist("/usr/bin/foo"), false);

   ASSERT_EQ(Path::createDir("", Path::getTempDirectory()), true);
   ASSERT_EQ(Path::exist(Path::getTempDirectory()), true);

   ASSERT_EQ(Path::createDir("tmp", ""), true);
   ASSERT_EQ(Path::exist("tmp"), true);
   std::remove("tmp");

   ASSERT_EQ(Path::createDir("", ""), false);
}

TEST(TestDir, createTempName)
{
   std::string dir = Path::createTempName("", "foo");
   std::cout << "dir: " << dir << std::endl;

   ASSERT_EQ(dir.empty(), false);
   ASSERT_EQ(dir[0] == '/', false);
   ASSERT_EQ(dir[0] == '\\', false);
   ASSERT_EQ(dir[1] == ':', false);

   dir = Path::createTempName(Path::getTempDirectory(), "foo");
   std::cout << "dir: " << dir << std::endl;

   ASSERT_EQ(Path::exist(dir), false);
   ASSERT_EQ(Path::isDir(dir), false);
   ASSERT_EQ(Path::isWritable(dir), false);
   ASSERT_EQ(Path::isReadable(dir), false);

   ASSERT_EQ(Path::createDir(dir, ""), true);
   ASSERT_EQ(Path::exist(dir), true);
   ASSERT_EQ(Path::isDir(dir), true);
   ASSERT_EQ(Path::isWritable(dir), true);
   ASSERT_EQ(Path::isReadable(dir), true);
}

TEST(TestDir, canonicalPath)
{
   ASSERT_STREQ(Path::canonicalPath("/foo/bar/file.txt").c_str(), "/foo/bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("./foo/bar/file.txt").c_str(), "./foo/bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("/foo/../bar/file.txt").c_str(), "/bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("./foo/../bar/file.txt").c_str(), "./bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("").c_str(), "");
   ASSERT_STREQ(Path::canonicalPath("..").c_str(), "..");
   ASSERT_STREQ(Path::canonicalPath("/").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("//").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("////").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("///.///").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("//.").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("/..").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("/out").c_str(), "/out");
   ASSERT_STREQ(Path::canonicalPath("./out").c_str(), "./out");
   ASSERT_STREQ(Path::canonicalPath("./././out").c_str(), "./out");
   ASSERT_STREQ(Path::canonicalPath("./out/./bin").c_str(), "./out/bin");
   ASSERT_STREQ(Path::canonicalPath("./out/./././bin").c_str(), "./out/bin");
   ASSERT_STREQ(Path::canonicalPath("out/../../bin").c_str(), "../bin");
   ASSERT_STREQ(Path::canonicalPath("../../bin").c_str(), "../../bin");
   ASSERT_STREQ(Path::canonicalPath("../..//bin").c_str(), "../../bin");
   ASSERT_STREQ(Path::canonicalPath("../.././bin").c_str(), "../../bin");
   ASSERT_STREQ(Path::canonicalPath("/../out/../in").c_str(), "/in");
   ASSERT_STREQ(Path::canonicalPath("/../out/../in/").c_str(), "/in");
   ASSERT_STREQ(Path::canonicalPath("/does/not/exist//data/somefolder").c_str(),
                "/does/not/exist/data/somefolder");
}

TEST(TestDir, normalize)
{
   ASSERT_STREQ(Path::normalize("A//B").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/B/").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/B//").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/./B").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/foo/../B").c_str(), "A/B");
}

// -----------------------------------------------------------------------------
// Tests for Windows paths
// -----------------------------------------------------------------------------
TEST(TestWindowsPaths, fileNameWindows)
{
   ASSERT_STREQ(Path::fileName("C:\\foo\\bar\\file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("C:\\foo\\bar\\file.foo.txt").c_str(), "file.foo.txt");
   ASSERT_STREQ(Path::fileName("C:\\foo\\bar").c_str(), "bar");
   ASSERT_STREQ(Path::fileName("C:\\foo\\bar\\").c_str(), "");
   ASSERT_STREQ(Path::fileName("C:\\Program Files\\App\\data.bin").c_str(), "data.bin");
   ASSERT_STREQ(Path::fileName("\\\\server\\share\\file.txt").c_str(), "file.txt");
}

TEST(TestWindowsPaths, dirNameWindows)
{
   ASSERT_STREQ(Path::dirName("C:\\foo\\bar\\file.txt").c_str(), "C:\\foo\\bar");
   ASSERT_STREQ(Path::dirName("C:\\foo\\bar\\").c_str(), "C:\\foo\\bar");
   ASSERT_STREQ(Path::dirName("C:\\foo\\bar").c_str(), "C:\\foo");
   ASSERT_STREQ(Path::dirName("C:\\foo").c_str(), "C:\\");
   ASSERT_STREQ(Path::dirName("C:\\").c_str(), "C:\\");
   ASSERT_STREQ(Path::dirName("\\\\server\\share\\folder").c_str(), "\\\\server\\share");
}

TEST(TestWindowsPaths, suffixWindows)
{
   ASSERT_STREQ(Path::extension("C:\\foo\\bar\\file.txt").c_str(), "txt");
   ASSERT_STREQ(Path::extension("C:\\foo\\bar\\file.foo.txt").c_str(), "foo.txt");
   ASSERT_STREQ(Path::extension("C:\\foo\\bar\\archive.tar.gz").c_str(), "tar.gz");
   ASSERT_STREQ(Path::extension("C:\\foo\\bar\\file").c_str(), "");
}

TEST(TestWindowsPaths, canonicalPathWindows)
{
   ASSERT_STREQ(Path::canonicalPath("C:\\foo\\..\\bar\\file.txt").c_str(), "C:\\bar\\file.txt");
   ASSERT_STREQ(Path::canonicalPath("C:\\foo\\.\\bar\\..\\baz").c_str(), "C:\\foo\\baz");
   ASSERT_STREQ(Path::canonicalPath("C:\\foo\\bar\\..\\..\\baz").c_str(), "C:\\baz");
   ASSERT_STREQ(Path::canonicalPath("C:\\.\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
   // TODO
   // ASSERT_STREQ(Path::canonicalPath("\\\\server\\share\\..\\other").c_str(), "\\\\server\\other");
}

TEST(TestWindowsPaths, normalizeWindows)
{
   ASSERT_STREQ(Path::normalize("C:\\foo\\\\bar").c_str(), "C:\\foo\\bar");
   ASSERT_STREQ(Path::normalize("C:\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
   ASSERT_STREQ(Path::normalize("C:\\foo\\bar\\..\\baz").c_str(), "C:\\foo\\baz");
   ASSERT_STREQ(Path::normalize("\\\\server\\\\share").c_str(), "\\\\server\\share");
}

// -----------------------------------------------------------------------------
// Tests for mixed paths (with both Windows and Unix separators)
// -----------------------------------------------------------------------------
TEST(TestMixedPaths, hasMixedSeparators)
{
   ASSERT_TRUE(Path::hasMixedSeparators("C:/foo\\bar/file.txt"));
   ASSERT_TRUE(Path::hasMixedSeparators("/usr\\local/bin"));
   ASSERT_FALSE(Path::hasMixedSeparators("C:\\foo\\bar\\file.txt"));
   ASSERT_FALSE(Path::hasMixedSeparators("/usr/local/bin"));
}

TEST(TestMixedPaths, fileNameMixed)
{
   ASSERT_STREQ(Path::fileName("C:/foo\\bar/file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("/usr\\local/bin\\app").c_str(), "app");
}

TEST(TestMixedPaths, dirNameMixed)
{
   ASSERT_STREQ(Path::dirName("C:/foo\\bar/file.txt").c_str(), "C:/foo\\bar");
   ASSERT_STREQ(Path::dirName("/usr\\local/bin\\app").c_str(), "/usr\\local/bin");
}

TEST(TestMixedPaths, normalizeMixed)
{
   ASSERT_STREQ(Path::normalize("C:/foo\\bar//\\file.txt").c_str(), "C:\\foo\\bar\\file.txt");
   ASSERT_STREQ(Path::normalize("/usr\\local/./bin\\\\app").c_str(), "/usr/local/bin/app");
}

// -----------------------------------------------------------------------------
// Tests for path conversion
// -----------------------------------------------------------------------------
TEST(TestPathConversion, toUnixSeparators)
{
   ASSERT_STREQ(Path::toUnixSeparators("C:\\foo\\bar\\file.txt").c_str(), "C:/foo/bar/file.txt");
   ASSERT_STREQ(Path::toUnixSeparators("/usr/local/bin").c_str(), "/usr/local/bin");
   ASSERT_STREQ(Path::toUnixSeparators("C:/foo\\bar/file.txt").c_str(), "C:/foo/bar/file.txt");
}

TEST(TestPathConversion, toWindowsSeparators)
{
   ASSERT_STREQ(Path::toWindowsSeparators("C:/foo/bar/file.txt").c_str(), "C:\\foo\\bar\\file.txt");
   ASSERT_STREQ(Path::toWindowsSeparators("/usr/local/bin").c_str(), "\\usr\\local\\bin");
   ASSERT_STREQ(Path::toWindowsSeparators("C:/foo\\bar/file.txt").c_str(), "C:\\foo\\bar\\file.txt");
}

TEST(TestPathConversion, toNativeSeparators)
{
#if defined(_WIN32)
   ASSERT_STREQ(Path::toNativeSeparators("C:/foo/bar/file.txt").c_str(), "C:\\foo\\bar\\file.txt");
   ASSERT_STREQ(Path::toNativeSeparators("/usr/local/bin").c_str(), "\\usr\\local\\bin");
#else
   ASSERT_STREQ(Path::toNativeSeparators("C:\\foo\\bar\\file.txt").c_str(), "C:/foo/bar/file.txt");
   ASSERT_STREQ(Path::toNativeSeparators("/usr/local/bin").c_str(), "/usr/local/bin");
#endif
}

TEST(TestPathConversion, toZipArchiveSeparators)
{
   ASSERT_STREQ(Path::toZipArchiveSeparators("C:\\foo\\bar\\file.txt").c_str(), "C:/foo/bar/file.txt");
   ASSERT_STREQ(Path::toZipArchiveSeparators("/usr/local/bin").c_str(), "/usr/local/bin");
   ASSERT_STREQ(Path::toZipArchiveSeparators("C:/foo\\bar/file.txt").c_str(), "C:/foo/bar/file.txt");
}

// -----------------------------------------------------------------------------
// Tests for root and isRoot functions
// -----------------------------------------------------------------------------
TEST(TestRoot, rootFunctions)
{
   ASSERT_TRUE(Path::isRoot("C:\\"));
   ASSERT_TRUE(Path::isRoot("D:\\"));
   ASSERT_TRUE(Path::isRoot("/"));
   ASSERT_FALSE(Path::isRoot("C:\\Windows"));
   ASSERT_STREQ(Path::root("C:\\Windows\\System32").c_str(), "C:\\");
   ASSERT_STREQ(Path::root("D:\\Program Files").c_str(), "D:\\");
   ASSERT_TRUE(Path::isRoot("/"));
   ASSERT_FALSE(Path::isRoot("/usr"));
   ASSERT_STREQ(Path::root("/usr/local/bin").c_str(), "/");
   ASSERT_FALSE(Path::isRoot("relative/path"));
}

// -----------------------------------------------------------------------------
// Tests for relative and absolute paths
// -----------------------------------------------------------------------------
TEST(TestRelativeAbsolute, isRelativePath)
{
   ASSERT_TRUE(Path::isRelativePath("relative/path"));
   ASSERT_TRUE(Path::isRelativePath("./foo/bar"));
   ASSERT_TRUE(Path::isRelativePath("../foo/bar"));
   ASSERT_FALSE(Path::isRelativePath("/usr/local/bin"));
   ASSERT_FALSE(Path::isRelativePath("C:\\Windows"));
   ASSERT_FALSE(Path::isRelativePath("C:/Program Files"));
}

#if 0
TEST(TestRelativeAbsolute, makePathRelative)
{
   std::string path = "/usr/local/bin";
   ASSERT_TRUE(Path::makePathRelative(path, "/usr"));
   ASSERT_STREQ(path.c_str(), "local/bin");

   path = "C:\\Windows\\System32";
   ASSERT_TRUE(Path::makePathRelative(path, "C:\\Windows"));
   ASSERT_STREQ(path.c_str(), "System32");
}

TEST(TestRelativeAbsolute, makePathAbsolute)
{
   std::string path = "local/bin";
   ASSERT_TRUE(Path::makePathAbsolute(path, "/usr"));
   ASSERT_STREQ(path.c_str(), "/usr/local/bin");

   path = "System32";
   ASSERT_TRUE(Path::makePathAbsolute(path, "C:\\Windows"));
   ASSERT_STREQ(path.c_str(), "C:\\Windows\\System32");
}
#endif

// -----------------------------------------------------------------------------
// Tests for folderNameWithSeparator function
// -----------------------------------------------------------------------------
TEST(TestDir, folderNameWithSeparator)
{
    // Tests on Linux (or with Unix separators)
    ASSERT_STREQ(Path::folderNameWithSeparator("/usr/local").c_str(), "/usr/local/");
    ASSERT_STREQ(Path::folderNameWithSeparator("/usr/local/").c_str(), "/usr/local/");
    ASSERT_STREQ(Path::folderNameWithSeparator("/tmp").c_str(), "/tmp/");
    ASSERT_STREQ(Path::folderNameWithSeparator("relative/path").c_str(), "relative/path/");
    ASSERT_STREQ(Path::folderNameWithSeparator("./foo").c_str(), "./foo/");
    ASSERT_STREQ(Path::folderNameWithSeparator("").c_str(), "/");

    // Tests with Windows paths
    ASSERT_STREQ(Path::folderNameWithSeparator("C:\\Windows").c_str(), "C:\\Windows/");
    ASSERT_STREQ(Path::folderNameWithSeparator("C:\\Windows\\").c_str(), "C:\\Windows/");
    ASSERT_STREQ(Path::folderNameWithSeparator("C:/Windows").c_str(), "C:/Windows/");
    ASSERT_STREQ(Path::folderNameWithSeparator("C:/Windows/").c_str(), "C:/Windows/");

    // Tests with mixed paths
    ASSERT_STREQ(Path::folderNameWithSeparator("C:/Program Files\\App").c_str(), "C:/Program Files\\App/");
    ASSERT_STREQ(Path::folderNameWithSeparator("/usr\\local/bin").c_str(), "/usr\\local/bin/");
}