#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "utils/Path.hpp"

#ifndef PWD
#    error "PWD is not defined"
#endif

using namespace zipper;

TEST(TestDir, exist)
{
    EXPECT_EQ(Path::exist(PWD), true);
    EXPECT_EQ(Path::exist(PWD "/main.cpp"), true);
    EXPECT_EQ(Path::exist(""), false);
    EXPECT_EQ(Path::exist("fsdfqfz"), false);
    EXPECT_EQ(Path::exist("fsdfqfz/"), false);
    EXPECT_EQ(Path::exist("fsdfqfz.txt"), false);
    EXPECT_EQ(Path::exist("fsdfqfz/sdsdsd.txt"), false);
    EXPECT_EQ(Path::exist("fsdfqfz/sdsdsd.txt/sdsdsd.txt"), false);
}

TEST(TestDir, isDir)
{
    EXPECT_EQ(Path::isDir(PWD), true);
    EXPECT_EQ(Path::isDir(PWD "/main.cpp"), false);
    EXPECT_EQ(Path::isDir(""), false);
    EXPECT_EQ(Path::isDir("sdsdqsd"), false);
}

TEST(TestDir, isFile)
{
    EXPECT_EQ(Path::isFile(PWD), false);
    EXPECT_EQ(Path::isFile(PWD "/main.cpp"), true);
    EXPECT_EQ(Path::isFile(""), false);
    EXPECT_EQ(Path::isFile("aazaza"), false);
}

TEST(TestDir, isReadable)
{
    EXPECT_EQ(Path::isReadable(PWD), true);
    EXPECT_EQ(Path::isReadable(PWD "/main.cpp"), true);
    EXPECT_EQ(Path::isReadable(""), false);
    EXPECT_EQ(Path::isReadable("qdqdqsdqdq"), false);
#if !defined(_WIN32)
    EXPECT_EQ(Path::isReadable("/usr/bin"), true);
#endif
}

TEST(TestDir, isWritable)
{
    EXPECT_EQ(Path::isWritable(PWD), true);
    EXPECT_EQ(Path::isWritable(PWD "/main.cpp"), true);
    EXPECT_EQ(Path::isWritable(""), false);
    EXPECT_EQ(Path::isWritable("qdqdqsdqdq"), false);
#if !defined(_WIN32)
    EXPECT_EQ(Path::isWritable("/usr/bin"), false);
#endif
}

TEST(TestDir, fileName)
{
    EXPECT_STREQ(Path::fileName("/foo/bar/file.txt").c_str(), "file.txt");
    EXPECT_STREQ(Path::fileName("/foo/bar/file.foo.txt").c_str(),
                 "file.foo.txt");
    EXPECT_STREQ(Path::fileName("/foo/bar").c_str(), "bar");
    EXPECT_STREQ(Path::fileName("/foo/bar/").c_str(), "");
    EXPECT_STREQ(Path::fileName("./foo/../bar/file.txt").c_str(), "file.txt");
    EXPECT_STREQ(Path::fileName("./foo/../bar/../file.txt").c_str(),
                 "file.txt");
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
    EXPECT_STREQ(Path::dirName("./foo/../bar/file.txt").c_str(),
                 "./foo/../bar");
    EXPECT_STREQ(Path::dirName("./foo/../bar/../file.txt").c_str(),
                 "./foo/../bar/..");
    EXPECT_STREQ(Path::dirName("/var/tmp/.").c_str(), "/var/tmp");
    ;
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
    std::string const c_barFoo(Path::getTempDirectory() + "/bar/foo");

    EXPECT_EQ(Path::exist(Path::getTempDirectory()), true);
    std::remove(c_fooBar.c_str());
    std::remove(c_barFoo.c_str());

    EXPECT_EQ(Path::createDir("bar/foo/", Path::getTempDirectory()), true);
    EXPECT_EQ(Path::exist(c_barFoo), true);
    EXPECT_EQ(Path::isDir(c_barFoo), true);
    EXPECT_EQ(Path::isWritable(c_barFoo), true);
    EXPECT_EQ(Path::isReadable(c_barFoo), true);

    EXPECT_EQ(Path::createDir("foo/bar", Path::getTempDirectory()), true);
    EXPECT_EQ(Path::exist(c_fooBar), true);
    EXPECT_EQ(Path::isDir(c_fooBar), true);
    EXPECT_EQ(Path::isWritable(c_fooBar), true);
    EXPECT_EQ(Path::isReadable(c_fooBar), true);

    std::remove("doesnotexist");
    EXPECT_EQ(Path::createDir("foo/bar", "doesnotexist"), true);
    EXPECT_EQ(Path::exist("doesnotexist/foo/bar"), true);
    std::remove("doesnotexist");

    // Permission denied
#if !defined(_WIN32)
    EXPECT_EQ(Path::createDir("foo", "/usr/bin"), false);
    EXPECT_EQ(Path::exist("/usr/bin/foo"), false);
#endif

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

TEST(TestDir, normalize)
{
    // Tests de base pour la normalisation des chemins
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
    // Tests pour les chemins avec ".."
    EXPECT_STREQ(Path::normalize("/../foo").c_str(), "/foo");
    EXPECT_STREQ(Path::normalize("/../../foo").c_str(), "/foo");
    EXPECT_STREQ(Path::normalize("bar/../foo").c_str(), "foo");
    EXPECT_STREQ(Path::normalize("bar/../../foo").c_str(), "../foo");
    EXPECT_STREQ(Path::normalize("/../").c_str(), "/");
    EXPECT_STREQ(Path::normalize("/a/../../").c_str(), "/");
    EXPECT_STREQ(Path::normalize("/a/b/../../").c_str(), "/");
}

TEST(TestDir, canonicalPath)
{
    // Tests pour les chemins canoniques
    EXPECT_STREQ(Path::normalize("/foo/bar/file.txt").c_str(),
                 "/foo/bar/file.txt");
    EXPECT_STREQ(Path::normalize("./foo/bar/file.txt").c_str(),
                 "foo/bar/file.txt");
    EXPECT_STREQ(Path::normalize("/foo/../bar/file.txt").c_str(),
                 "/bar/file.txt");
    EXPECT_STREQ(Path::normalize("./foo/../bar/file.txt").c_str(),
                 "bar/file.txt");
    EXPECT_STREQ(Path::normalize("").c_str(), "");
    EXPECT_STREQ(Path::normalize("..").c_str(), "..");
    EXPECT_STREQ(Path::normalize("/").c_str(), "/");
    EXPECT_STREQ(Path::normalize("//").c_str(), "/");
    EXPECT_STREQ(Path::normalize("////").c_str(), "/");
    EXPECT_STREQ(Path::normalize("///.///").c_str(), "/");
    EXPECT_STREQ(Path::normalize("//.").c_str(), "/");
    EXPECT_STREQ(Path::normalize("/..").c_str(), "/");
    EXPECT_STREQ(Path::normalize("/out").c_str(), "/out");
    EXPECT_STREQ(Path::normalize("./out").c_str(), "out");
    EXPECT_STREQ(Path::normalize("./././out").c_str(), "out");
    EXPECT_STREQ(Path::normalize("./out/./bin").c_str(), "out/bin");
    EXPECT_STREQ(Path::normalize("./out/./././bin").c_str(), "out/bin");
    EXPECT_STREQ(Path::normalize("out/../../bin").c_str(), "../bin");
    EXPECT_STREQ(Path::normalize("../../bin").c_str(), "../../bin");
    EXPECT_STREQ(Path::normalize("../..//bin").c_str(), "../../bin");
    EXPECT_STREQ(Path::normalize("../.././bin").c_str(), "../../bin");
    EXPECT_STREQ(Path::normalize("/../out/../in").c_str(), "/in");
    EXPECT_STREQ(Path::normalize("/../out/../in/").c_str(), "/in");
    EXPECT_STREQ(Path::normalize("/does/not/exist//data/somefolder").c_str(),
                 "/does/not/exist/data/somefolder");
    EXPECT_STREQ(Path::normalize("/does/not/exist//data/somefolder/").c_str(),
                 "/does/not/exist/data/somefolder");
    EXPECT_STREQ(Path::normalize("/does/not/exist//data/somefolder//").c_str(),
                 "/does/not/exist/data/somefolder");
}

TEST(TestDir, canonicalPathExtended)
{
    // Additional tests for base cases
    EXPECT_STREQ(Path::normalize("./").c_str(), ".");
    EXPECT_STREQ(Path::normalize("././").c_str(), ".");
    EXPECT_STREQ(Path::normalize("./.").c_str(), ".");
    EXPECT_STREQ(Path::normalize("./../").c_str(), "..");
    EXPECT_STREQ(Path::normalize("../..").c_str(), "../..");

    // Cases with multiple separators and cleaning
    EXPECT_STREQ(Path::normalize("/foo//bar").c_str(), "/foo/bar");
    EXPECT_STREQ(Path::normalize("/foo///bar").c_str(), "/foo/bar");
    EXPECT_STREQ(Path::normalize("/foo/./bar").c_str(), "/foo/bar");
    EXPECT_STREQ(Path::normalize("/./foo/bar").c_str(), "/foo/bar");

    // Gestion des ".." consécutifs
    EXPECT_STREQ(Path::normalize("/foo/bar/../..").c_str(), "/");
    EXPECT_STREQ(Path::normalize("/foo/bar/../../baz").c_str(), "/baz");
    EXPECT_STREQ(Path::normalize("../../../foo").c_str(), "../../../foo");

    // Tests avec séparateurs mixtes (Windows/Unix)
    EXPECT_STREQ(Path::normalize("/foo/bar\\/baz").c_str(), "/foo/bar/baz");
    EXPECT_STREQ(Path::normalize("/foo\\bar/baz").c_str(), "/foo/bar/baz");

    // Cas spéciaux avec des points
    EXPECT_STREQ(Path::normalize("/foo/./bar/.").c_str(), "/foo/bar");
    EXPECT_STREQ(Path::normalize("/foo/././bar").c_str(), "/foo/bar");
    EXPECT_STREQ(Path::normalize("/foo/./../bar").c_str(), "/bar");

    // Cas avec segments vides
    EXPECT_STREQ(Path::normalize("//foo///bar//").c_str(), "/foo/bar");
    EXPECT_STREQ(Path::normalize("foo//bar//").c_str(), "foo/bar");

    // Tests avec chemins Windows
    EXPECT_STREQ(Path::normalize("C:\\foo\\..\\bar").c_str(), "C:\\bar");
    EXPECT_STREQ(Path::normalize("C:/foo/../bar").c_str(), "C:\\bar");
    EXPECT_STREQ(Path::normalize("C:\\..\\foo").c_str(), "C:\\foo");
    EXPECT_STREQ(Path::normalize("C:\\.\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
}

TEST(TestWindowsPaths, canonicalPathWindows)
{
    EXPECT_STREQ(Path::normalize("C:\\foo\\..\\bar\\file.txt").c_str(),
                 "C:\\bar\\file.txt");
    EXPECT_STREQ(Path::normalize("C:\\foo\\.\\bar\\..\\baz").c_str(),
                 "C:\\foo\\baz");
    EXPECT_STREQ(Path::normalize("C:\\foo\\bar\\..\\..\\baz").c_str(),
                 "C:\\baz");
    EXPECT_STREQ(Path::normalize("C:\\.\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
    EXPECT_STREQ(Path::normalize("C:\\foo\\\\bar").c_str(), "C:\\foo\\bar");
    EXPECT_STREQ(Path::normalize("C:\\foo\\.\\bar").c_str(), "C:\\foo\\bar");
    EXPECT_STREQ(Path::normalize("C:\\foo\\bar\\..\\baz").c_str(),
                 "C:\\foo\\baz");
}

TEST(TestMixedPaths, normalizeMixed)
{
    EXPECT_STREQ(Path::normalize("C:/foo\\bar//\\file.txt").c_str(),
                 "C:\\foo\\bar\\file.txt");
    EXPECT_STREQ(Path::normalize("/usr\\local/./bin\\\\app").c_str(),
                 "/usr/local/bin/app");
}

#if 0 // hard to distinguish with Linux paths
// -----------------------------------------------------------------------------
// Tests for UNC paths normalization
// -----------------------------------------------------------------------------
TEST(TestUNCPaths, normalizeUNC)
{
    // Simple UNC root
    EXPECT_STREQ(Path::normalize("\\\\server\\share").c_str(),
                 "\\\\server\\share");
    EXPECT_STREQ(Path::normalize("//server/share").c_str(),
                 "\\\\server\\share");

    // UNC with subfolders
    EXPECT_STREQ(Path::normalize("\\\\server\\share\\folder\\file.txt").c_str(),
                 "\\\\server\\share\\folder\\file.txt");
    EXPECT_STREQ(Path::normalize("//server/share/folder/file.txt").c_str(),
                 "\\\\server\\share\\folder\\file.txt");

    // UNC with mixed separators
    EXPECT_STREQ(Path::normalize("\\\\server/share\\folder//file.txt").c_str(),
                 "\\\\server\\share\\folder\\file.txt");

    // UNC with parent directory navigation
    EXPECT_STREQ(
        Path::normalize("\\\\server\\share\\folder\\..\\other").c_str(),
        "\\\\server\\other");
    EXPECT_STREQ(Path::normalize("//server/share/folder/../other").c_str(),
                 "\\\\server\\other");

    // UNC with current directory navigation
    EXPECT_STREQ(
        Path::normalize("\\\\server\\share\\.\\folder\\.\\file.txt").c_str(),
        "\\\\server\\share\\folder\\file.txt");

    // UNC with multiple consecutive slashes
    EXPECT_STREQ(
        Path::normalize("\\\\server\\\\share\\\\folder\\\\file.txt").c_str(),
        "\\\\server\\share\\folder\\file.txt");

    // UNC root only
    EXPECT_STREQ(Path::normalize("\\\\server\\share\\..").c_str(),
                 "\\\\server");
    EXPECT_STREQ(Path::normalize("//server/share/..").c_str(), "/server");
}
#endif

// -----------------------------------------------------------------------------
// Tests for Windows paths
// -----------------------------------------------------------------------------
TEST(TestWindowsPaths, fileNameWindows)
{
    EXPECT_STREQ(Path::fileName("C:\\foo\\bar\\file.txt").c_str(), "file.txt");
    EXPECT_STREQ(Path::fileName("C:\\foo\\bar\\file.foo.txt").c_str(),
                 "file.foo.txt");
    EXPECT_STREQ(Path::fileName("C:\\foo\\bar").c_str(), "bar");
    EXPECT_STREQ(Path::fileName("C:\\foo\\bar\\").c_str(), "");
    EXPECT_STREQ(Path::fileName("C:\\Program Files\\App\\data.bin").c_str(),
                 "data.bin");
    EXPECT_STREQ(Path::fileName("\\\\server\\share\\file.txt").c_str(),
                 "file.txt");
}

TEST(TestWindowsPaths, dirNameWindows)
{
    EXPECT_STREQ(Path::dirName("C:\\foo\\bar\\file.txt").c_str(),
                 "C:\\foo\\bar");
    EXPECT_STREQ(Path::dirName("C:\\foo\\bar\\").c_str(), "C:\\foo\\bar");
    EXPECT_STREQ(Path::dirName("C:\\foo\\bar").c_str(), "C:\\foo");
    EXPECT_STREQ(Path::dirName("C:\\foo").c_str(), "C:\\");
    EXPECT_STREQ(Path::dirName("C:\\").c_str(), "C:\\");
    EXPECT_STREQ(Path::dirName("\\\\server\\share\\folder").c_str(),
                 "\\\\server\\share");
}

TEST(TestWindowsPaths, suffixWindows)
{
    EXPECT_STREQ(Path::extension("C:\\foo\\bar\\file.txt").c_str(), "txt");
    EXPECT_STREQ(Path::extension("C:\\foo\\bar\\file.foo.txt").c_str(),
                 "foo.txt");
    EXPECT_STREQ(Path::extension("C:\\foo\\bar\\archive.tar.gz").c_str(),
                 "tar.gz");
    EXPECT_STREQ(Path::extension("C:\\foo\\bar\\file").c_str(), "");
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
    EXPECT_STREQ(Path::dirName("/usr\\local/bin\\app").c_str(),
                 "/usr\\local/bin");
}

// -----------------------------------------------------------------------------
// Tests for path conversion
// -----------------------------------------------------------------------------
TEST(TestPathConversion, toUnixSeparators)
{
    EXPECT_STREQ(Path::toUnixSeparators("C:\\foo\\bar\\file.txt").c_str(),
                 "C:/foo/bar/file.txt");
    EXPECT_STREQ(Path::toUnixSeparators("/usr/local/bin").c_str(),
                 "/usr/local/bin");
    EXPECT_STREQ(Path::toUnixSeparators("C:/foo\\bar/file.txt").c_str(),
                 "C:/foo/bar/file.txt");
}

TEST(TestPathConversion, toWindowsSeparators)
{
    EXPECT_STREQ(Path::toWindowsSeparators("C:/foo/bar/file.txt").c_str(),
                 "C:\\foo\\bar\\file.txt");
    EXPECT_STREQ(Path::toWindowsSeparators("/usr/local/bin").c_str(),
                 "\\usr\\local\\bin");
    EXPECT_STREQ(Path::toWindowsSeparators("C:/foo\\bar/file.txt").c_str(),
                 "C:\\foo\\bar\\file.txt");
}

TEST(TestPathConversion, toNativeSeparators)
{
#if defined(_WIN32)
    EXPECT_STREQ(Path::toNativeSeparators("C:/foo/bar/file.txt").c_str(),
                 "C:\\foo\\bar\\file.txt");
    EXPECT_STREQ(Path::toNativeSeparators("/usr/local/bin").c_str(),
                 "\\usr\\local\\bin");
#else
    EXPECT_STREQ(Path::toNativeSeparators("C:\\foo\\bar\\file.txt").c_str(),
                 "C:/foo/bar/file.txt");
    EXPECT_STREQ(Path::toNativeSeparators("/usr/local/bin").c_str(),
                 "/usr/local/bin");
#endif
}

TEST(TestPathConversion, toZipArchiveSeparators)
{
    EXPECT_STREQ(Path::toZipArchiveSeparators("C:\\foo\\bar\\file.txt").c_str(),
                 "C:/foo/bar/file.txt");
    EXPECT_STREQ(Path::toZipArchiveSeparators("/usr/local/bin").c_str(),
                 "/usr/local/bin");
    EXPECT_STREQ(Path::toZipArchiveSeparators("C:/foo\\bar/file.txt").c_str(),
                 "C:/foo/bar/file.txt");
}

// -----------------------------------------------------------------------------
// Tests for root and hasRoot functions
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
    EXPECT_STREQ(Path::folderNameWithSeparator("/usr/local").c_str(),
                 "/usr/local/");
    EXPECT_STREQ(Path::folderNameWithSeparator("/usr/local/").c_str(),
                 "/usr/local/");
    EXPECT_STREQ(Path::folderNameWithSeparator("/tmp").c_str(), "/tmp/");
    EXPECT_STREQ(Path::folderNameWithSeparator("relative/path").c_str(),
                 "relative/path/");
    EXPECT_STREQ(Path::folderNameWithSeparator("./foo").c_str(), "./foo/");
    EXPECT_STREQ(Path::folderNameWithSeparator("").c_str(), "/");

    // Tests with Windows paths
    EXPECT_STREQ(Path::folderNameWithSeparator("C:\\Windows").c_str(),
                 "C:\\Windows/");
    EXPECT_STREQ(Path::folderNameWithSeparator("C:\\Windows\\").c_str(),
                 "C:\\Windows/");
    EXPECT_STREQ(Path::folderNameWithSeparator("C:/Windows").c_str(),
                 "C:/Windows/");
    EXPECT_STREQ(Path::folderNameWithSeparator("C:/Windows/").c_str(),
                 "C:/Windows/");

    // Tests with mixed paths
    EXPECT_STREQ(Path::folderNameWithSeparator("C:/Program Files\\App").c_str(),
                 "C:/Program Files\\App/");
    EXPECT_STREQ(Path::folderNameWithSeparator("/usr\\local/bin").c_str(),
                 "/usr\\local/bin/");
}

// -----------------------------------------------------------------------------
// Zip Slip attack detection (GoogleTest version)
// -----------------------------------------------------------------------------
TEST(TestSlipAttack, ZipSlipDetection)
{
    // Safe paths
    EXPECT_FALSE(Path::isZipSlip("file.txt", "/safe/dir"));
    EXPECT_FALSE(Path::isZipSlip("subdir/file.txt", "/safe/dir"));
    EXPECT_FALSE(Path::isZipSlip("./sub/file", "/safe/dir"));
    EXPECT_FALSE(Path::isZipSlip("file.txt", "/safe/dir/"));

    // Explicit attacks
    EXPECT_TRUE(Path::isZipSlip("../evil.txt", "/safe/dir"));
    EXPECT_TRUE(Path::isZipSlip("../evil.txt", "/safe/dir/"));
    EXPECT_TRUE(Path::isZipSlip("../../../../etc/passwd", "/safe/dir"));
    EXPECT_TRUE(Path::isZipSlip("../../../../etc/passwd", "/safe/dir/"));
    EXPECT_TRUE(Path::isZipSlip("/absolute/evil", "/safe/dir"));
    EXPECT_TRUE(Path::isZipSlip("/absolute/evil", ""));
    EXPECT_FALSE(Path::isZipSlip("/absolute/evil", "/"));
    EXPECT_FALSE(Path::isZipSlip("/absolute/not/evil", "/"));
    EXPECT_FALSE(Path::isZipSlip("absolute/not/evil", "/"));

    // Nasty case
    // https://www.sonarsource.com/blog/openrefine-zip-slip/
    EXPECT_TRUE(Path::isZipSlip("/home/johnny/.ssh/id_rsa", "/home/john"));
    EXPECT_TRUE(Path::isZipSlip("/home/johnny/.ssh/id_rsa", "/home/john/"));
    EXPECT_FALSE(Path::isZipSlip("ny/.ssh/id_rsa", "/home/john"));
    EXPECT_FALSE(Path::isZipSlip("ny/.ssh/id_rsa", "/home/john/"));

    // Edge cases
    EXPECT_FALSE(Path::isZipSlip("", "/safe/dir"));
    EXPECT_FALSE(Path::isZipSlip("subdir/../legal.txt", "/safe/dir"));
    EXPECT_TRUE(Path::isZipSlip("subdir/../../evil.txt", "/safe/dir"));
    EXPECT_FALSE(Path::isZipSlip("a/b/c/../../evil.txt", "/safe/dir/"));

    // Windows-style paths
    EXPECT_TRUE(Path::isZipSlip("..\\evil.txt", "C:\\safe\\dir"));
    EXPECT_TRUE(Path::isZipSlip("C:\\evil.txt", "C:\\safe\\dir"));

    // Test with current path
    EXPECT_FALSE(Path::isZipSlip("issue_05/", ""));
    EXPECT_FALSE(Path::isZipSlip("issue_05/Nouveau dossier/", ""));
    EXPECT_FALSE(Path::isZipSlip("issue_05/Nouveau fichier vide", ""));
    EXPECT_FALSE(Path::isZipSlip("issue_05/foo/", ""));
    EXPECT_FALSE(Path::isZipSlip("issue_05/foo/bar", ""));

    // Test with current path
    EXPECT_FALSE(Path::isZipSlip("issue_05/", "."));
    EXPECT_FALSE(Path::isZipSlip("issue_05/Nouveau dossier/", "."));
    EXPECT_FALSE(Path::isZipSlip("issue_05/Nouveau fichier vide", "."));
    EXPECT_FALSE(Path::isZipSlip("issue_05/foo/", "."));
    EXPECT_FALSE(Path::isZipSlip("issue_05/foo/bar", "."));

    // Test with current path
    EXPECT_FALSE(Path::isZipSlip("issue_05/", "./"));
    EXPECT_FALSE(Path::isZipSlip("issue_05/Nouveau dossier/", "./"));
    EXPECT_FALSE(Path::isZipSlip("issue_05/Nouveau fichier vide", "./"));
    EXPECT_FALSE(Path::isZipSlip("issue_05/foo/", "./"));
    EXPECT_FALSE(Path::isZipSlip("issue_05/foo/bar", "./"));
}