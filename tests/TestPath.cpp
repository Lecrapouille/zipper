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
   EXPECT_EQ(Path::exist(PWD), true);
   EXPECT_EQ(Path::exist(PWD"/main.cpp"), true);
   EXPECT_EQ(Path::exist(""), false);
   EXPECT_EQ(Path::exist("fsdfqfz"), false);
}

TEST(TestDir, isDir)
{
   EXPECT_EQ(Path::isDir(PWD), true);
   EXPECT_EQ(Path::isDir(PWD"/main.cpp"), false);
   EXPECT_EQ(Path::isDir(""), false);
   EXPECT_EQ(Path::isDir("sdsdqsd"), false);
}

TEST(TestDir, isFile)
{
   EXPECT_EQ(Path::isFile(PWD), false);
   EXPECT_EQ(Path::isFile(PWD"/main.cpp"), true);
   EXPECT_EQ(Path::isFile(""), false);
   EXPECT_EQ(Path::isFile("aazaza"), false);
}

TEST(TestDir, isReadable)
{
   EXPECT_EQ(Path::isReadable(PWD), true);
   EXPECT_EQ(Path::isReadable(PWD"/main.cpp"), true);
   EXPECT_EQ(Path::isReadable(""), false);
   EXPECT_EQ(Path::isReadable("qdqdqsdqdq"), false);
#if ! defined(_WIN32)
   EXPECT_EQ(Path::isReadable("/usr/bin"), true);
#endif
}

TEST(TestDir, isWritable)
{
   EXPECT_EQ(Path::isWritable(PWD), true);
   EXPECT_EQ(Path::isWritable(PWD"/main.cpp"), true);
   EXPECT_EQ(Path::isWritable(""), false);
   EXPECT_EQ(Path::isWritable("qdqdqsdqdq"), false);
#if ! defined(_WIN32)
   EXPECT_EQ(Path::isWritable("/usr/bin"), false);
#endif
}

TEST(TestDir, fileName)
{
   EXPECT_STREQ(Path::fileName("/foo/bar/file.txt").c_str(), "file.txt");
   EXPECT_STREQ(Path::fileName("/foo/bar/file.foo.txt").c_str(), "file.foo.txt");
   EXPECT_STREQ(Path::fileName("/foo/bar").c_str(), "bar");
   EXPECT_STREQ(Path::fileName("/foo/bar/").c_str(), "");
   EXPECT_STREQ(Path::fileName("./foo/../bar/file.txt").c_str(), "file.txt");
   EXPECT_STREQ(Path::fileName("./foo/../bar/../file.txt").c_str(), "file.txt");
   EXPECT_STREQ(Path::fileName("").c_str(), "");
   EXPECT_STREQ(Path::fileName("..").c_str(), "..");
   EXPECT_STREQ(Path::fileName("/").c_str(), "");
   EXPECT_STREQ(Path::fileName("//").c_str(), "");
   EXPECT_STREQ(Path::fileName("//.").c_str(), ".");

   EXPECT_STREQ(Path::fileName("/foo/bar.txt").c_str(), "bar.txt");
   EXPECT_STREQ(Path::fileName("/foo/.bar").c_str(), ".bar");
   EXPECT_STREQ(Path::fileName("/foo/bar/").c_str(), "");
   EXPECT_STREQ(Path::fileName("/foo/.").c_str(), ".");
   EXPECT_STREQ(Path::fileName("/foo/..").c_str(), "..");
   EXPECT_STREQ(Path::fileName(".").c_str(), ".");
   EXPECT_STREQ(Path::fileName("..").c_str(), "..");
   EXPECT_STREQ(Path::fileName("/").c_str(), "");
   EXPECT_STREQ(Path::fileName("//host").c_str(), "host");
}

TEST(TestDir, dirName)
{
   EXPECT_STREQ(Path::dirName("/foo/bar/file.txt").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::dirName("/foo/bar/file.foo.txt").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::dirName("/foo/bar").c_str(), "/foo");
   EXPECT_STREQ(Path::dirName("/foo/bar/").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::dirName("./foo/../bar/file.txt").c_str(), "./foo/../bar");
   EXPECT_STREQ(Path::dirName("./foo/../bar/../file.txt").c_str(), "./foo/../bar/..");
   EXPECT_STREQ(Path::dirName("/var/tmp/.").c_str(), "/var/tmp");;
   EXPECT_STREQ(Path::dirName("/usr/lib").c_str(), "/usr");
   EXPECT_STREQ(Path::dirName("/usr/").c_str(), "/usr");
   EXPECT_STREQ(Path::dirName("/usr").c_str(), "/");
   EXPECT_STREQ(Path::dirName("usr").c_str(), "");
   EXPECT_STREQ(Path::dirName("/").c_str(), "/");
   EXPECT_STREQ(Path::dirName(".").c_str(), "");
   EXPECT_STREQ(Path::dirName("..").c_str(), "");
   EXPECT_STREQ(Path::dirName("//").c_str(), "/");
   EXPECT_STREQ(Path::dirName("//.").c_str(), "/");
}

TEST(TestDir, extension)
{
   EXPECT_STREQ(Path::extension("/foo/bar/file.txt").c_str(), "txt");
   EXPECT_STREQ(Path::extension("/foo/bar/file.foo.txt").c_str(), "foo.txt");
   EXPECT_STREQ(Path::extension(".txt").c_str(), "txt");
   EXPECT_STREQ(Path::extension("/a/b.c/d").c_str(), "");
   EXPECT_STREQ(Path::extension("").c_str(), "");
   EXPECT_STREQ(Path::extension("txt").c_str(), "");
   EXPECT_STREQ(Path::extension("foo.bar.baz").c_str(), "bar.baz");
   EXPECT_STREQ(Path::extension(".bar.baz.txt").c_str(), "bar.baz.txt");
   EXPECT_STREQ(Path::extension(".").c_str(), "");
}

TEST(TestDir, createDir)
{
   std::string const c_fooBar(Path::getTempDirectory() + "/foo/bar");

   EXPECT_EQ(Path::exist(Path::getTempDirectory()), true);
   std::remove(c_fooBar.c_str());

   EXPECT_EQ(Path::createDir("foo/bar", Path::getTempDirectory()), true);
   EXPECT_EQ(Path::exist(c_fooBar), true);
   EXPECT_EQ(Path::isDir(c_fooBar), true);
   EXPECT_EQ(Path::isWritable(c_fooBar), true);
   EXPECT_EQ(Path::isReadable(c_fooBar), true);

   EXPECT_EQ(Path::createDir("foo/bar", "doesnotexist"), false);
   EXPECT_EQ(Path::exist("doesnotexist/foo/bar"), false);

   EXPECT_EQ(Path::createDir("foo", "/usr/bin"), false);
   EXPECT_EQ(Path::exist("/usr/bin/foo"), false);

   EXPECT_EQ(Path::createDir("", Path::getTempDirectory()), true);
   EXPECT_EQ(Path::exist(Path::getTempDirectory()), true);

   EXPECT_EQ(Path::createDir("tmp", ""), true);
   EXPECT_EQ(Path::exist("tmp"), true);
   std::remove("tmp");

   EXPECT_EQ(Path::createDir("", ""), false);
}

TEST(TestDir, createTempName)
{
   std::string dir = Path::createTempName("", "foo");
   std::cout << "dir: " << dir << std::endl;

   EXPECT_EQ(dir.empty(), false);
   EXPECT_EQ(dir[0] == '/', false);
   EXPECT_EQ(dir[0] == '\\', false);
   EXPECT_EQ(dir[1] == ':', false);

   dir = Path::createTempName(Path::getTempDirectory(), "foo");
   std::cout << "dir: " << dir << std::endl;

   EXPECT_EQ(Path::exist(dir), false);
   EXPECT_EQ(Path::isDir(dir), false);
   EXPECT_EQ(Path::isWritable(dir), false);
   EXPECT_EQ(Path::isReadable(dir), false);

   EXPECT_EQ(Path::createDir(dir, ""), true);
   EXPECT_EQ(Path::exist(dir), true);
   EXPECT_EQ(Path::isDir(dir), true);
   EXPECT_EQ(Path::isWritable(dir), true);
   EXPECT_EQ(Path::isReadable(dir), true);
}

TEST(TestDir, canonicalPath)
{
   EXPECT_STREQ(Path::canonicalPath("/foo/bar/file.txt").c_str(), "/foo/bar/file.txt");
   EXPECT_STREQ(Path::canonicalPath("./foo/bar/file.txt").c_str(), "./foo/bar/file.txt");
   EXPECT_STREQ(Path::canonicalPath("/foo/../bar/file.txt").c_str(), "/bar/file.txt");
   EXPECT_STREQ(Path::canonicalPath("./foo/../bar/file.txt").c_str(), "./bar/file.txt");
   EXPECT_STREQ(Path::canonicalPath("").c_str(), "");
   EXPECT_STREQ(Path::canonicalPath("..").c_str(), "..");
   EXPECT_STREQ(Path::canonicalPath("/").c_str(), "/");
   EXPECT_STREQ(Path::canonicalPath("//").c_str(), "/");
   EXPECT_STREQ(Path::canonicalPath("////").c_str(), "/");
   EXPECT_STREQ(Path::canonicalPath("///.///").c_str(), "/");
   EXPECT_STREQ(Path::canonicalPath("//.").c_str(), "/");
   EXPECT_STREQ(Path::canonicalPath("/..").c_str(), "/");
   EXPECT_STREQ(Path::canonicalPath("/out").c_str(), "/out");
   EXPECT_STREQ(Path::canonicalPath("./out").c_str(), "./out");
   EXPECT_STREQ(Path::canonicalPath("./././out").c_str(), "./out");
   EXPECT_STREQ(Path::canonicalPath("./out/./bin").c_str(), "./out/bin");
   EXPECT_STREQ(Path::canonicalPath("./out/./././bin").c_str(), "./out/bin");
   EXPECT_STREQ(Path::canonicalPath("out/../../bin").c_str(), "../bin");
   EXPECT_STREQ(Path::canonicalPath("../../bin").c_str(), "../../bin");
   EXPECT_STREQ(Path::canonicalPath("../..//bin").c_str(), "../../bin");
   EXPECT_STREQ(Path::canonicalPath("../.././bin").c_str(), "../../bin");
   EXPECT_STREQ(Path::canonicalPath("/../out/../in").c_str(), "/in");
   EXPECT_STREQ(Path::canonicalPath("/../out/../in/").c_str(), "/in");
   EXPECT_STREQ(Path::canonicalPath("/does/not/exist//data/somefolder").c_str(),
                "/does/not/exist/data/somefolder");
   EXPECT_STREQ(Path::canonicalPath("/does/not/exist//data/somefolder/").c_str(),
                "/does/not/exist/data/somefolder");
   EXPECT_STREQ(Path::canonicalPath("/does/not/exist//data/somefolder//").c_str(),
                "/does/not/exist/data/somefolder");
}

TEST(TestDir, canonicalPathExtended)
{

   // Additional base tests
   EXPECT_STREQ(Path::canonicalPath("./").c_str(), ".");
   EXPECT_STREQ(Path::canonicalPath("././").c_str(), ".");
   EXPECT_STREQ(Path::canonicalPath("./.").c_str(), ".");
   EXPECT_STREQ(Path::canonicalPath("./../").c_str(), "..");
   EXPECT_STREQ(Path::canonicalPath("../..").c_str(), "../..");

   // Case with multiple separators and cleaning
   EXPECT_STREQ(Path::canonicalPath("/foo//bar").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::canonicalPath("/foo///bar").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::canonicalPath("/foo/./bar").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::canonicalPath("/./foo/bar").c_str(), "/foo/bar");

   // Handling consecutive '..'
   EXPECT_STREQ(Path::canonicalPath("/foo/bar/../..").c_str(), "/");
   EXPECT_STREQ(Path::canonicalPath("/foo/bar/../../baz").c_str(), "/baz");
   EXPECT_STREQ(Path::canonicalPath("../../../foo").c_str(), "../../../foo");

   // Tests with mixed separators (Windows/Unix)
   EXPECT_STREQ(Path::canonicalPath("/foo/bar\\/baz").c_str(), "/foo/bar/baz");
   EXPECT_STREQ(Path::canonicalPath("/foo\\bar/baz").c_str(), "/foo/bar/baz");

   // Special cases with dots
   EXPECT_STREQ(Path::canonicalPath("/foo/./bar/.").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::canonicalPath("/foo/././bar").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::canonicalPath("/foo/./../bar").c_str(), "/bar");

   // Case with empty segments
   EXPECT_STREQ(Path::canonicalPath("//foo///bar//").c_str(), "/foo/bar");
   EXPECT_STREQ(Path::canonicalPath("foo//bar//").c_str(), "foo/bar");

   // Tests with Windows paths
   EXPECT_STREQ(Path::canonicalPath("C:\\foo\\..\\bar").c_str(), "C:\\bar");
   EXPECT_STREQ(Path::canonicalPath("C:/foo/../bar").c_str(), "C:\\bar");
   EXPECT_STREQ(Path::canonicalPath("C:\\..\\foo").c_str(), "C:\\foo");
   EXPECT_STREQ(Path::canonicalPath("C:\\.\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
}

#if 0
TEST(TestDir, normalize)
{
   EXPECT_STREQ(Path::normalize("A//B").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("A/B/").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("A/B//").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("A/./B").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("A/foo/../B").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("./A/B").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("A/B/.").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("A/B/./").c_str(), "A/B");
   EXPECT_STREQ(Path::normalize("A/B/./C").c_str(), "A/B/C");
   EXPECT_STREQ(Path::normalize("A/B/./C/").c_str(), "A/B/C");
}

TEST(TestDir, normalizeSpecialCase)
{
   EXPECT_STREQ(Path::normalize("/../foo").c_str(), "/foo");
   EXPECT_STREQ(Path::normalize("/../../foo").c_str(), "/foo");
   EXPECT_STREQ(Path::normalize("bar/../foo").c_str(), "bar/../foo");
   EXPECT_STREQ(Path::normalize("bar/../../foo").c_str(), "../foo");
   EXPECT_STREQ(Path::normalize("/../").c_str(), "/");
   EXPECT_STREQ(Path::normalize("/a/../../").c_str(), "/");
   EXPECT_STREQ(Path::normalize("/a/b/../../").c_str(), "/");
}
#endif

// -----------------------------------------------------------------------------
// Tests for Windows paths
// -----------------------------------------------------------------------------
TEST(TestWindowsPaths, fileNameWindows)
{
   EXPECT_STREQ(Path::fileName("C:\\foo\\bar\\file.txt").c_str(), "file.txt");
   EXPECT_STREQ(Path::fileName("C:\\foo\\bar\\file.foo.txt").c_str(), "file.foo.txt");
   EXPECT_STREQ(Path::fileName("C:\\foo\\bar").c_str(), "bar");
   EXPECT_STREQ(Path::fileName("C:\\foo\\bar\\").c_str(), "");
   EXPECT_STREQ(Path::fileName("C:\\Program Files\\App\\data.bin").c_str(), "data.bin");
   EXPECT_STREQ(Path::fileName("\\\\server\\share\\file.txt").c_str(), "file.txt");
}

TEST(TestWindowsPaths, dirNameWindows)
{
   EXPECT_STREQ(Path::dirName("C:\\foo\\bar\\file.txt").c_str(), "C:\\foo\\bar");
   EXPECT_STREQ(Path::dirName("C:\\foo\\bar\\").c_str(), "C:\\foo\\bar");
   EXPECT_STREQ(Path::dirName("C:\\foo\\bar").c_str(), "C:\\foo");
   EXPECT_STREQ(Path::dirName("C:\\foo").c_str(), "C:\\");
   EXPECT_STREQ(Path::dirName("C:\\").c_str(), "C:\\");
   EXPECT_STREQ(Path::dirName("\\\\server\\share\\folder").c_str(), "\\\\server\\share");
}

TEST(TestWindowsPaths, suffixWindows)
{
   EXPECT_STREQ(Path::extension("C:\\foo\\bar\\file.txt").c_str(), "txt");
   EXPECT_STREQ(Path::extension("C:\\foo\\bar\\file.foo.txt").c_str(), "foo.txt");
   EXPECT_STREQ(Path::extension("C:\\foo\\bar\\archive.tar.gz").c_str(), "tar.gz");
   EXPECT_STREQ(Path::extension("C:\\foo\\bar\\file").c_str(), "");
}

TEST(TestWindowsPaths, canonicalPathWindows)
{
   EXPECT_STREQ(Path::canonicalPath("C:\\foo\\..\\bar\\file.txt").c_str(), "C:\\bar\\file.txt");
   EXPECT_STREQ(Path::canonicalPath("C:\\foo\\.\\bar\\..\\baz").c_str(), "C:\\foo\\baz");
   EXPECT_STREQ(Path::canonicalPath("C:\\foo\\bar\\..\\..\\baz").c_str(), "C:\\baz");
   EXPECT_STREQ(Path::canonicalPath("C:\\.\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
   // TODO
   // EXPECT_STREQ(Path::canonicalPath("\\\\server\\share\\..\\other").c_str(), "\\\\server\\other");
}

TEST(TestWindowsPaths, normalizeWindows)
{
   EXPECT_STREQ(Path::normalize("C:\\foo\\\\bar").c_str(), "C:\\foo\\bar");
   EXPECT_STREQ(Path::normalize("C:\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
   EXPECT_STREQ(Path::normalize("C:\\foo\\bar\\..\\baz").c_str(), "C:\\foo\\baz");
   EXPECT_STREQ(Path::normalize("\\\\server\\\\share").c_str(), "\\\\server\\share");
}

// -----------------------------------------------------------------------------
// Tests for mixed paths (with both Windows and Unix separators)
// -----------------------------------------------------------------------------
TEST(TestMixedPaths, hasMixedSeparators)
{
   EXPECT_TRUE(Path::hasMixedSeparators("C:/foo\\bar/file.txt"));
   EXPECT_TRUE(Path::hasMixedSeparators("/usr\\local/bin"));
   EXPECT_FALSE(Path::hasMixedSeparators("C:\\foo\\bar\\file.txt"));
   EXPECT_FALSE(Path::hasMixedSeparators("/usr/local/bin"));
}

TEST(TestMixedPaths, fileNameMixed)
{
   EXPECT_STREQ(Path::fileName("C:/foo\\bar/file.txt").c_str(), "file.txt");
   EXPECT_STREQ(Path::fileName("/usr\\local/bin\\app").c_str(), "app");
}

TEST(TestMixedPaths, dirNameMixed)
{
   EXPECT_STREQ(Path::dirName("C:/foo\\bar/file.txt").c_str(), "C:/foo\\bar");
   EXPECT_STREQ(Path::dirName("/usr\\local/bin\\app").c_str(), "/usr\\local/bin");
}

TEST(TestMixedPaths, normalizeMixed)
{
   EXPECT_STREQ(Path::normalize("C:/foo\\bar//\\file.txt").c_str(), "C:\\foo\\bar\\file.txt");
   EXPECT_STREQ(Path::normalize("/usr\\local/./bin\\\\app").c_str(), "/usr/local/bin/app");
}

// -----------------------------------------------------------------------------
// Tests for path conversion
// -----------------------------------------------------------------------------
TEST(TestPathConversion, toUnixSeparators)
{
   EXPECT_STREQ(Path::toUnixSeparators("C:\\foo\\bar\\file.txt").c_str(), "C:/foo/bar/file.txt");
   EXPECT_STREQ(Path::toUnixSeparators("/usr/local/bin").c_str(), "/usr/local/bin");
   EXPECT_STREQ(Path::toUnixSeparators("C:/foo\\bar/file.txt").c_str(), "C:/foo/bar/file.txt");
}

TEST(TestPathConversion, toWindowsSeparators)
{
   EXPECT_STREQ(Path::toWindowsSeparators("C:/foo/bar/file.txt").c_str(), "C:\\foo\\bar\\file.txt");
   EXPECT_STREQ(Path::toWindowsSeparators("/usr/local/bin").c_str(), "\\usr\\local\\bin");
   EXPECT_STREQ(Path::toWindowsSeparators("C:/foo\\bar/file.txt").c_str(), "C:\\foo\\bar\\file.txt");
}

TEST(TestPathConversion, toNativeSeparators)
{
#if defined(_WIN32)
   EXPECT_STREQ(Path::toNativeSeparators("C:/foo/bar/file.txt").c_str(), "C:\\foo\\bar\\file.txt");
   EXPECT_STREQ(Path::toNativeSeparators("/usr/local/bin").c_str(), "\\usr\\local\\bin");
#else
   EXPECT_STREQ(Path::toNativeSeparators("C:\\foo\\bar\\file.txt").c_str(), "C:/foo/bar/file.txt");
   EXPECT_STREQ(Path::toNativeSeparators("/usr/local/bin").c_str(), "/usr/local/bin");
#endif
}

TEST(TestPathConversion, toZipArchiveSeparators)
{
   EXPECT_STREQ(Path::toZipArchiveSeparators("C:\\foo\\bar\\file.txt").c_str(), "C:/foo/bar/file.txt");
   EXPECT_STREQ(Path::toZipArchiveSeparators("/usr/local/bin").c_str(), "/usr/local/bin");
   EXPECT_STREQ(Path::toZipArchiveSeparators("C:/foo\\bar/file.txt").c_str(), "C:/foo/bar/file.txt");
}

// -----------------------------------------------------------------------------
// Tests for root and isRoot functions
// -----------------------------------------------------------------------------
TEST(TestRoot, rootFunctions)
{
   EXPECT_TRUE(Path::isRoot("C:\\"));
   EXPECT_TRUE(Path::isRoot("D:\\"));
   EXPECT_TRUE(Path::isRoot("/"));
   EXPECT_FALSE(Path::isRoot("C:\\Windows"));
   EXPECT_STREQ(Path::root("C:\\Windows\\System32").c_str(), "C:\\");
   EXPECT_STREQ(Path::root("D:\\Program Files").c_str(), "D:\\");
   EXPECT_TRUE(Path::isRoot("/"));
   EXPECT_FALSE(Path::isRoot("/usr"));
   EXPECT_STREQ(Path::root("/usr/local/bin").c_str(), "/");
   EXPECT_FALSE(Path::isRoot("relative/path"));
}

// -----------------------------------------------------------------------------
// Tests for relative and absolute paths
// -----------------------------------------------------------------------------
TEST(TestRelativeAbsolute, isRelativePath)
{
   EXPECT_TRUE(Path::isRelativePath("relative/path"));
   EXPECT_TRUE(Path::isRelativePath("./foo/bar"));
   EXPECT_TRUE(Path::isRelativePath("../foo/bar"));
   EXPECT_FALSE(Path::isRelativePath("/usr/local/bin"));
   EXPECT_FALSE(Path::isRelativePath("C:\\Windows"));
   EXPECT_FALSE(Path::isRelativePath("C:/Program Files"));
}

#if 0
TEST(TestRelativeAbsolute, makePathRelative)
{
   std::string path = "/usr/local/bin";
   EXPECT_TRUE(Path::makePathRelative(path, "/usr"));
   EXPECT_STREQ(path.c_str(), "local/bin");

   path = "C:\\Windows\\System32";
   EXPECT_TRUE(Path::makePathRelative(path, "C:\\Windows"));
   EXPECT_STREQ(path.c_str(), "System32");
}

TEST(TestRelativeAbsolute, makePathAbsolute)
{
   std::string path = "local/bin";
   EXPECT_TRUE(Path::makePathAbsolute(path, "/usr"));
   EXPECT_STREQ(path.c_str(), "/usr/local/bin");

   path = "System32";
   EXPECT_TRUE(Path::makePathAbsolute(path, "C:\\Windows"));
   EXPECT_STREQ(path.c_str(), "C:\\Windows\\System32");
}
#endif

// -----------------------------------------------------------------------------
// Tests for folderNameWithSeparator function
// -----------------------------------------------------------------------------
TEST(TestDir, folderNameWithSeparator)
{
    // Tests on Linux (or with Unix separators)
    EXPECT_STREQ(Path::folderNameWithSeparator("/usr/local").c_str(), "/usr/local/");
    EXPECT_STREQ(Path::folderNameWithSeparator("/usr/local/").c_str(), "/usr/local/");
    EXPECT_STREQ(Path::folderNameWithSeparator("/tmp").c_str(), "/tmp/");
    EXPECT_STREQ(Path::folderNameWithSeparator("relative/path").c_str(), "relative/path/");
    EXPECT_STREQ(Path::folderNameWithSeparator("./foo").c_str(), "./foo/");
    EXPECT_STREQ(Path::folderNameWithSeparator("").c_str(), "/");

    // Tests with Windows paths
    EXPECT_STREQ(Path::folderNameWithSeparator("C:\\Windows").c_str(), "C:\\Windows/");
    EXPECT_STREQ(Path::folderNameWithSeparator("C:\\Windows\\").c_str(), "C:\\Windows/");
    EXPECT_STREQ(Path::folderNameWithSeparator("C:/Windows").c_str(), "C:/Windows/");
    EXPECT_STREQ(Path::folderNameWithSeparator("C:/Windows/").c_str(), "C:/Windows/");

    // Tests with mixed paths
    EXPECT_STREQ(Path::folderNameWithSeparator("C:/Program Files\\App").c_str(), "C:/Program Files\\App/");
    EXPECT_STREQ(Path::folderNameWithSeparator("/usr\\local/bin").c_str(), "/usr\\local/bin/");
}