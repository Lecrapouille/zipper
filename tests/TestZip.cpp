#if 0
#    include <boost/interprocess/streams/vectorstream.hpp>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#pragma GCC diagnostic pop

#include "utils/Path.hpp"
#include <chrono>
#include <fstream>

#define protected public
#define private public
#include "Zipper/Unzipper.hpp"
#include "Zipper/Zipper.hpp"
#undef protected
#undef private

#ifndef PWD
#    error "PWD is not defined"
#endif

using namespace zipper;

// -----------------------------------------------------------------------------
static bool createFile(const char* file, const char* content)
{
    std::ofstream ofs(file);
    ofs << content;
    ofs.flush();
    ofs.close();

    return Path::exist(file) && Path::isFile(file);
}

// -----------------------------------------------------------------------------
static std::string readFileContent(const char* file)
{
    std::ifstream ifs(file);
    std::string str((std::istreambuf_iterator<char>(ifs)),
                    std::istreambuf_iterator<char>());
    return str.c_str();
}

// -----------------------------------------------------------------------------
static bool helperZipEntry(Zipper& zipper,
                           const char* filepath,
                           const char* content,
                           const char* entrypath)
{
    std::ofstream ofs(filepath);
    ofs << content;
    ofs.flush();
    ofs.close();

    std::ifstream ifs(filepath);
    bool res = zipper.add(ifs, entrypath, Zipper::SaveHierarchy);
    ifs.close();

    Path::remove(filepath);

    return res;
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipperNominalOpenings)
{
    // Clean up.
    Path::remove("ziptest.zip");
    ASSERT_EQ(Path::exist("ziptest.zip"), false);

    // Create a fake file (not zip file, just the extension).
    std::ofstream outfile("ziptest.zip");
    outfile.close();

    // Fake file exists ?
    ASSERT_EQ(Path::exist("ziptest.zip"), true);
    ASSERT_EQ(Path::isFile("ziptest.zip"), true);

    // Create a zip. The fake file is replaced.
    Zipper zipper1("ziptest.zip", Zipper::OpenFlags::Overwrite);
    ASSERT_EQ(helperZipEntry(
                  zipper1, "test1.txt", "test1 file compression", "test1.txt"),
              true);
    zipper1.close();

    // Check the fake file has been replaced.
    zipper::Unzipper unzipper1("ziptest.zip");
    std::vector<zipper::ZipEntry> entries1 = unzipper1.entries();
    ASSERT_EQ(entries1.size(), 1u);
    ASSERT_STREQ(entries1[0].name.c_str(), "test1.txt");
    unzipper1.close();

    // Replace the zip
    Zipper zipper2("ziptest.zip", Zipper::OpenFlags::Overwrite);
    zipper2.close();

    // Check the fake file has been replaced: "test1.txt" does not exist.
    ASSERT_EQ(Path::exist("ziptest.zip"), true);
    ASSERT_EQ(Path::isFile("ziptest.zip"), true);
    zipper::Unzipper unzipper2("ziptest.zip");
    std::vector<zipper::ZipEntry> entries2 = unzipper2.entries();
    unzipper2.close();
    ASSERT_EQ(entries2.size(), 0u);

    // Create a zip file with "test1.txt" file
    Zipper zipper3("ziptest.zip", Zipper::OpenFlags::Overwrite);
    ASSERT_EQ(helperZipEntry(
                  zipper3, "test1.txt", "test1 file compression", "test1.txt"),
              true);
    zipper3.close();

    // Replace the zip file with "test2.txt" file
    Zipper zipper4("ziptest.zip", Zipper::OpenFlags::Overwrite);
    ASSERT_EQ(helperZipEntry(
                  zipper4, "test2.txt", "test2 file compression", "test2.txt"),
              true);
    zipper4.close();

    // Check if test2.txt has replaced test1.txt
    zipper::Unzipper unzipper4("ziptest.zip");
    std::vector<zipper::ZipEntry> entries4 = unzipper4.entries();
    unzipper4.close();
    ASSERT_EQ(entries4.size(), 1u);
    ASSERT_STREQ(entries4[0].name.c_str(), "test2.txt");

    // Append zip file
    Zipper zipper5("ziptest.zip", Zipper::OpenFlags::Append);
    ASSERT_EQ(helperZipEntry(
                  zipper5, "test1.txt", "test1 file compression", "test1.txt"),
              true);
    zipper5.close();

    // Check if test2.txt and test1.txt exist
    zipper::Unzipper unzipper5("ziptest.zip");
    std::vector<zipper::ZipEntry> entries5 = unzipper5.entries();
    unzipper5.close();
    ASSERT_EQ(entries5.size(), 2u);
    ASSERT_STREQ(entries5[0].name.c_str(), "test2.txt");
    ASSERT_STREQ(entries5[1].name.c_str(), "test1.txt");

    // Reopen and append zip entries
    ASSERT_EQ(zipper5.open(), true); // Default behavior: append
    ASSERT_EQ(helperZipEntry(
                  zipper5, "test3.txt", "test3 file compression", "test3.txt"),
              true);
    zipper5.close();

    zipper::Unzipper unzipper5_1("ziptest.zip");
    std::vector<zipper::ZipEntry> entries5_1 = unzipper5_1.entries();
    unzipper5_1.close();
    ASSERT_EQ(entries5_1.size(), 3u);
    ASSERT_STREQ(entries5_1[0].name.c_str(), "test2.txt");
    ASSERT_STREQ(entries5_1[1].name.c_str(), "test1.txt");
    ASSERT_STREQ(entries5_1[2].name.c_str(), "test3.txt");

    // Reopen and erase zip entries
    ASSERT_EQ(zipper5.open(Zipper::OpenFlags::Overwrite), true);
    ASSERT_EQ(helperZipEntry(
                  zipper5, "test3.txt", "test3 file compression", "test3.txt"),
              true);
    zipper5.close();

    zipper::Unzipper unzipper5_2("ziptest.zip");
    std::vector<zipper::ZipEntry> entries5_2 = unzipper5_2.entries();
    unzipper5_2.close();
    ASSERT_EQ(entries5_2.size(), 1u);
    ASSERT_STREQ(entries5_2[0].name.c_str(), "test3.txt");

    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipperPathologicalOpenings)
{
    std::string folder;
#if !defined(_WIN32)
    // Opening a folder
    folder = "/usr/bin";
#else
    folder = "C:\\Windows";
#endif
    ASSERT_EQ(Path::exist(folder), true);
    ASSERT_EQ(Path::isDir(folder), true);
    try
    {
        Zipper zipper(folder);
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Is a directory");
    }

#if !defined(_WIN32)
    // Permission denied
    ASSERT_EQ(Path::exist("/usr/bin/ziptest.zip"), false);
    try
    {
        Zipper zipper("/usr/bin/ziptest.zip");
    }
    catch (std::runtime_error const& e)
    {
#    if defined(__APPLE__)
        std::string s1 = "Read-only file system";
        std::string s2 = "Operation not permitted";
        std::string str = e.what();
        EXPECT_PRED3(
            [](auto str, auto s1, auto s2) { return str == s1 || str == s2; },
            str,
            s1,
            s2);
#    else
        ASSERT_STREQ(e.what(), "Permission denied");
#    endif
    }
#endif
}

// -----------------------------------------------------------------------------
TEST(FileUnzipTests, UnzipperPathologicalOpenings)
{
    // Opening a non existing file
    ASSERT_EQ(Path::exist("doesnotexist"), false);
    try
    {
        Unzipper unzipper("doesnotexist");
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Does not exist");
    }

    // TODO This test works with MyMakefile but not with CMake.
    // We have to find a generic way
#if 0
    // Opening a non zip file
    ASSERT_EQ(Path::exist("../build/zipper-tests"), true);
    ASSERT_EQ(Path::isFile("../build/zipper-tests"), true);
    try
    {
        Unzipper unzipper("../build/zipper-tests");
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Not a zip file");
    }
#endif

    // Opening a non zip file
#if !defined(_WIN32)
#    define NON_ZIP_FILE "/usr/bin/make"
#else
#    define NON_ZIP_FILE "C:\\Windows\\System32\\cmd.exe"
#endif

    ASSERT_EQ(Path::exist(NON_ZIP_FILE), true);
    ASSERT_EQ(Path::isFile(NON_ZIP_FILE), true);
    try
    {
        Unzipper unzipper(NON_ZIP_FILE);
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Not a zip file");
    }

    // Opening a folder
#if !defined(_WIN32)
#    define FOLDER "/usr/bin"
#else
#    define FOLDER "C:\\Windows"
#endif

    ASSERT_EQ(Path::exist(FOLDER), true);
    ASSERT_EQ(Path::isDir(FOLDER), true);
    try
    {
        Unzipper unzipper(FOLDER);
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Not a zip file");
    }

    // TODO Real zip + permission denied
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipfileFeedWithDifferentInputs1)
{
    // Clean up
    Path::remove("ziptest.zip");

    // Zip a file named 'test1' containing 'test file compression'
    Zipper zipper("ziptest.zip");
    ASSERT_EQ(helperZipEntry(
                  zipper, "test1.txt", "test file compression", "test1.txt"),
              true);
    zipper.close();

    // Check if the zip file has one entry named 'test1.txt'
    zipper::Unzipper unzipper1("ziptest.zip");
    std::vector<zipper::ZipEntry> entries = unzipper1.entries();
    ASSERT_EQ(entries.size(), 1u);
    ASSERT_STREQ(entries[0].name.c_str(), "test1.txt");

    // And then extracting the test1.txt entry creates a file named 'test1.txt'
    // with the text 'test file compression'
    ASSERT_EQ(unzipper1.extractEntry("test1.txt"), true);
    unzipper1.close();
    ASSERT_EQ(Path::exist("test1.txt"), true);
    ASSERT_EQ(Path::isFile("test1.txt"), true);
    ASSERT_STREQ(readFileContent("test1.txt").c_str(), "test file compression");

    // Zip a second file named 'test2.dat' containing 'other data to compression
    // test' inside a folder 'TestFolder'
    ASSERT_EQ(zipper.open(), true);
    ASSERT_EQ(helperZipEntry(zipper,
                             "test2.dat",
                             "other data to compression test",
                             "TestFolder/test2.dat"),
              true);
    zipper.close();

    // Check the zip has two entries named 'test1.txt' and
    // 'TestFolder/test2.dat'
    zipper::Unzipper unzipper2("ziptest.zip");
    ASSERT_EQ(unzipper2.entries().size(), 2u);
    ASSERT_STREQ(unzipper2.entries()[0].name.c_str(), "test1.txt");
    ASSERT_STREQ(unzipper2.entries()[1].name.c_str(), "TestFolder/test2.dat");

    // Failed extracting since test1.txt is already present.
    ASSERT_EQ(unzipper2.extractAll(), false);
    ASSERT_STREQ(unzipper2.error().message().c_str(),
                 "Security Error: 'test1.txt' already exists and would have "
                 "been replaced!");

    // Extract the zip. Check the test2.dat entry creates a folder 'TestFolder'
    // with a file named 'test2.dat' with the text 'other data to compression
    // test'
    ASSERT_EQ(unzipper2.extractAll(true), true); // replace the "test1.txt"
    unzipper2.close();
    ASSERT_EQ(Path::exist("TestFolder/test2.dat"), true);
    ASSERT_EQ(Path::isFile("TestFolder/test2.dat"), true);
    ASSERT_STREQ(readFileContent("TestFolder/test2.dat").c_str(),
                 "other data to compression test");

    // And when adding a folder to the zip, creates one entry for each file
    // inside the folder with the name in zip as 'Folder/...'
    Path::createDir(Path::currentPath() + "/TestFiles/subfolder");
    ASSERT_EQ(createFile("TestFiles/test1.txt", "test file compression"), true);
    ASSERT_EQ(createFile("TestFiles/test2.pdf", "pdf file compression"), true);
    ASSERT_EQ(createFile("TestFiles/subfolder/test-sub.txt",
                         "test-sub file compression"),
              true);
    ASSERT_EQ(zipper.open(), true);
    ASSERT_EQ(zipper.add("TestFiles", Zipper::SaveHierarchy), true);
    zipper.close();

    zipper::Unzipper unzipper3("ziptest.zip");
    ASSERT_EQ(unzipper3.entries().size(), 5u);

    // And then extracting to a new folder 'NewDestination' creates the file
    // structure from zip in the new destination folder
    Path::remove(Path::currentPath() + "/NewDestination");
    Path::createDir(Path::currentPath() + "/NewDestination");
    ASSERT_EQ(unzipper3.extractAll(Path::currentPath() + "/NewDestination"),
              true);

    std::vector<std::string> files =
        Path::filesFromDir(Path::currentPath() + "/NewDestination", true);
    ASSERT_STREQ(readFileContent("NewDestination/TestFiles/test1.txt").c_str(),
                 "test file compression");
    ASSERT_STREQ(readFileContent("NewDestination/TestFiles/test2.pdf").c_str(),
                 "pdf file compression");
    ASSERT_STREQ(
        readFileContent("NewDestination/TestFiles/subfolder/test-sub.txt")
            .c_str(),
        "test-sub file compression");
    unzipper3.close();

    // Clean up
    Path::remove("TestFolder");
    Path::remove("TestFiles");
    Path::remove("NewDestination");
    Path::remove("test1.txt");
    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipfileFeedWithDifferentInputs2)
{
    // Clean up
    Path::remove("ziptest.zip");

    // Add a stringstream named 'strdata' containing 'test string data
    // compression' is added.
    Zipper zipper("ziptest.zip");
    ASSERT_EQ(helperZipEntry(
                  zipper, "strdata", "test string data compression", "strdata"),
              true);
    zipper.close();

    // Check the zip file has one entry named 'strdata'
    zipper::Unzipper unzipper("ziptest.zip");
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "strdata");

    // Extracting the strdata entry creates a file named 'strdata' with the text
    // 'test string data compression'"
    ASSERT_EQ(unzipper.extractAll(), true);
    ASSERT_EQ(Path::exist("strdata"), true);
    ASSERT_EQ(Path::isFile("strdata"), true);
    ASSERT_STREQ(readFileContent("strdata").c_str(),
                 "test string data compression");

    // Extracting with an alternative name 'alternative_strdata.dat' crates a
    // file with that name instead of the one inside de zip".
    std::map<std::string, std::string> alt_names;
    alt_names["strdata"] = "alternative_strdata.dat";
    ASSERT_EQ(unzipper.extractAll("", alt_names), true);
    ASSERT_EQ(Path::exist("alternative_strdata.dat"), true);
    ASSERT_EQ(Path::isFile("alternative_strdata.dat"), true);
    ASSERT_STREQ(readFileContent("alternative_strdata.dat").c_str(),
                 "test string data compression");

    // Trying to extract a file 'fake.dat' that doesn't exists, returns false
    ASSERT_EQ(unzipper.extractEntry("fake.dat"), false);
    ASSERT_EQ(Path::exist("fake.dat"), false);

    unzipper.close();
    Path::remove("strdata");
    Path::remove("alternative_strdata.dat");
    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
TEST(MemoryZipTests, ZipVectorFeedWithDifferentInputs1)
{
    // A Zip outputted to a vector
    std::vector<unsigned char> zipvec;
    zipper::Zipper zipper(zipvec);

    // A file containing 'test file compression' is added and named 'test1'
    std::ofstream test1("test1.txt");
    test1 << "test file compression";
    test1.flush();
    test1.close();

    std::ifstream test1stream("test1.txt");
    zipper.add(test1stream, "test1.txt", Zipper::SaveHierarchy);
    test1stream.close();
    zipper.close();

    // Reopen the zip
    zipper::Zipper zipper2(zipvec);
    zipper2.close();

    // Unzip the zip
    zipper::Unzipper unzipper(zipvec);

    // Check if the zip vector has one entry named 'test1.txt'
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "test1.txt");

    // Extracting the test1.txt entry creates a file named 'test1.txt' with the
    // text 'test file compression'
    Path::remove("test1.txt");
    ASSERT_EQ(unzipper.extractEntry("test1.txt"), true);
    // due to sections forking or creating different stacks we need to make sure
    // the local instance is closed to prevent mixing the closing when both
    // instances are freed at the end of the scope
    unzipper.close();

    ASSERT_EQ(Path::exist("test1.txt"), true);

    std::ifstream testfile("test1.txt");
    ASSERT_EQ(testfile.good(), true);

    std::string test((std::istreambuf_iterator<char>(testfile)),
                     std::istreambuf_iterator<char>());
    testfile.close();
    ASSERT_STREQ(test.c_str(), "test file compression");

    // Another file containing 'other data to compression test' and named
    // 'test2.dat' is added inside a folder 'TestFolder'
    std::ofstream test2("test2.dat");
    test2 << "other data to compression test";
    test2.flush();
    test2.close();

    std::ifstream test2stream("test2.dat");

    zipper.open();
    zipper.add(test2stream, "TestFolder/test2.dat", Zipper::SaveHierarchy);
    zipper.close();

    test2stream.close();
    Path::remove("test2.dat");

    zipper::Unzipper unzipper2(zipvec);

    // The zip vector has two entries named 'test1.txt' and
    // 'TestFolder/test2.dat'
    ASSERT_EQ(unzipper2.entries().size(), 2u);
    ASSERT_STREQ(unzipper2.entries()[0].name.c_str(), "test1.txt");
    ASSERT_STREQ(unzipper2.entries()[1].name.c_str(), "TestFolder/test2.dat");

    // Failed extracting since test1.txt is already present.
    try
    {
        ASSERT_EQ(unzipper2.extractAll(), false);
    }
    catch (std::runtime_error const& /*e*/)
    {
    }

    // Extracting the test2.dat entry creates a folder 'TestFolder' with a file
    // named 'test2.dat' with the text 'other data to compression test'
    ASSERT_EQ(unzipper2.extractAll(true), true);
    ASSERT_EQ(Path::exist("TestFolder/test2.dat"), true);

    std::ifstream testfile2("TestFolder/test2.dat");
    ASSERT_EQ(testfile2.good(), true);

    std::string test3((std::istreambuf_iterator<char>(testfile2)),
                      std::istreambuf_iterator<char>());
    testfile2.close();
    ASSERT_STREQ(test3.c_str(), "other data to compression test");

    // Extracting the test2.dat entry to memory fills a vector with 'other data
    // to compression test'
    std::vector<unsigned char> resvec;
    unzipper2.extractEntryToMemory("TestFolder/test2.dat", resvec);
    unzipper2.close();

    std::string test4(resvec.begin(), resvec.end());

    ASSERT_STREQ(test4.c_str(), "other data to compression test");
    unzipper2.close();

    Path::remove("test1.txt");
    Path::remove("TestFolder");

    zipvec.clear();
    Path::remove("TestFolder");
}

// -----------------------------------------------------------------------------
TEST(MemoryZipTests, ZipVectorFeedWithDifferentInputs2)
{
    // A Zip outputted to a vector
    std::vector<unsigned char> zipvec;
    zipper::Zipper zipper(zipvec);

    // A stringstream containing 'test string data compression' is added and
    // named 'strdata'
    std::stringstream strdata;
    strdata << "test string data compression";

    zipper.add(strdata, "strdata", Zipper::SaveHierarchy);
    zipper.close();

    // The zip vector has one entry named 'strdata'
    zipper::Unzipper unzipper(zipvec);
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "strdata");

    // Extracting the strdata entry creates a file named 'strdata' with the txt
    // 'test string data compression'
    ASSERT_EQ(unzipper.extractAll(true), true);
    ASSERT_EQ(Path::exist("strdata"), true);

    std::ifstream testfile3("strdata");
    ASSERT_EQ(testfile3.good(), true);

    std::string test5((std::istreambuf_iterator<char>(testfile3)),
                      std::istreambuf_iterator<char>());
    testfile3.close();
    ASSERT_STREQ(test5.c_str(), "test string data compression");

    // Extracting the strdata entry to memory, fills a vector with the txt 'test
    // string data compression'
    std::vector<unsigned char> resvec1;
    ASSERT_EQ(unzipper.extractEntryToMemory("strdata", resvec1), true);
    unzipper.close();

    std::string test6(resvec1.begin(), resvec1.end());
    ASSERT_STREQ(test6.c_str(), "test string data compression");

    Path::remove("strdata");
    zipvec.clear();

    // A file containing 'test file compression' is added and named 'test1' in
    // subdirectory 'subdirectory'
    bool fileWasCreated = Path::createDir("subdirectory");
    ASSERT_EQ(fileWasCreated, true);

    std::ofstream test7("./subdirectory/test1.txt");
    test7 << "test file compression";
    test7.flush();
    test7.close();

    Zipper::ZipFlags flags =
        Zipper::ZipFlags::Better | Zipper::ZipFlags::SaveHierarchy;
    zipper.add("./subdirectory/test1.txt", flags);

    zipper.close();

    Path::remove("./subdirectory/test1.txt");

#if 0
    // A Zip outputted to a vector;
    zipper::Unzipper unzipper2(zipvec);

    // The zip vector has entry named './subdirectory/test1.txt'
    ASSERT_EQ(unzipper2.entries().size(), 1u);
    ASSERT_STREQ(unzipper2.entries()[0].name.c_str(), "./subdirectory/test1.txt");

    // Extracting the test1.txt entry creates a file named 'test1.txt' with the
    // text 'test file compression'
    ASSERT_EQ(unzipper2.extractEntry("./subdirectory/test1.txt"), true);
    // due to sections forking or creating different stacks we need to make sure
    // the local instance is closed to prevent mixing the closing when both
    // instances are freed at the end of the scope
    unzipper2.close();

    ASSERT_EQ(Path::exist("./subdirectory/test1.txt"), true);
    std::ifstream testfile1("./subdirectory/test1.txt");
    ASSERT_EQ(testfile1.good(), true);

    std::string test8((std::istreambuf_iterator<char>(testfile1)),
                      std::istreambuf_iterator<char>());
    testfile1.close();
    ASSERT_STREQ(test8.c_str(), "test file compression");

    Path::remove("subdirectory");
    zipvec.clear();
#endif

    Path::remove("TestFolder");
    Path::remove("subdirectory");
}

// -----------------------------------------------------------------------------
TEST(MemoryZipTests, DummyVectorTest)
{
    std::vector<unsigned char> zipvec;
    try
    {
        zipper::Unzipper unzipper(zipvec);
        FAIL() << "An exception shall have thrown";
    }
    catch (std::runtime_error const& /*e*/)
    {
    }
}

// -----------------------------------------------------------------------------
TEST(ZipTests, PasswordTest)
{
    std::ofstream test1("test1.txt");
    test1 << "test file1 compression";
    test1.flush();
    test1.close();

    std::ofstream test2("test2.txt");
    test2 << "test file2 compression";
    test2.flush();
    test2.close();

    std::ifstream input1("test1.txt");
    std::ifstream input2("test2.txt");

    Zipper zipper("ziptest.zip", "123456");
    ASSERT_EQ(zipper.add(input1, "Test1", Zipper::SaveHierarchy), true);
    ASSERT_EQ(zipper.add(input2, "Test2", Zipper::SaveHierarchy), true);
    Path::remove("test1.txt");
    Path::remove("test2.txt");
    zipper.close();

    zipper::Unzipper unzipper("ziptest.zip", "123456");
    ASSERT_EQ(unzipper.entries().size(), 2u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "Test1");
    ASSERT_STREQ(unzipper.entries()[1].name.c_str(), "Test2");

    ASSERT_EQ(unzipper.extractAll(), true);
    ASSERT_EQ(Path::exist("Test1"), true);
    ASSERT_EQ(Path::isFile("Test1"), true);
    ASSERT_EQ(Path::exist("Test2"), true);
    ASSERT_EQ(Path::isFile("Test2"), true);

    std::ifstream testfile1("Test1");
    ASSERT_EQ(testfile1.good(), true);
    std::string test3((std::istreambuf_iterator<char>(testfile1)),
                      std::istreambuf_iterator<char>());
    testfile1.close();
    ASSERT_STREQ(test3.c_str(), "test file1 compression");

    std::ifstream testfile2("Test2");
    ASSERT_EQ(testfile2.good(), true);
    std::string test4((std::istreambuf_iterator<char>(testfile2)),
                      std::istreambuf_iterator<char>());
    testfile2.close();
    ASSERT_STREQ(test4.c_str(), "test file2 compression");

    Path::remove("ziptest.zip");
    Path::remove("Test1");
    Path::remove("Test2");
}

// -----------------------------------------------------------------------------
TEST(ZipTests, UnzipDummyTarball)
{
    // Clean up
    Path::remove("ziptest.zip");
    Path::remove("data");

    // Create folder
    Path::createDir("data/somefolder/");

    // Test with the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder/", Zipper::SaveHierarchy),
                  true); // With the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 0u);
        Path::remove("ziptest.zip");
    }

    // Test without the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder", Zipper::SaveHierarchy),
                  true); // Without the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 0u);
    }

    Path::remove("ziptest.zip");
    Path::remove("data");
}

// -----------------------------------------------------------------------------
TEST(ZipTests, ZipStreamNominal)
{
    // Clean up
    Path::remove("Test1");
    Path::remove("ziptest.zip");

    {
        std::stringstream ss;

        Zipper zipper(ss); // TODO password
        ASSERT_EQ(helperZipEntry(zipper, "somefile", "helloworld", "Test1"),
                  true);
        zipper.close();

        zipper::Unzipper unzipper(ss);
        ASSERT_EQ(unzipper.extractEntry("Test1"), true);
        unzipper.close();

        ASSERT_EQ(Path::exist("Test1"), true);
        ASSERT_EQ(Path::isFile("Test1"), true);
        ASSERT_STREQ(readFileContent("Test1").c_str(), "helloworld");

        Path::remove("Test1");
        Path::remove("ziptest.zip");
    }
}

#if 0
// -----------------------------------------------------------------------------
TEST(ZipTests, ZipVectorNominal)
{
    // Clean up
    Path::remove("Test1");

    {
        boost::interprocess::basic_vectorstream<std::vector<char>> zip_in_memory;

        Zipper zipper(zip_in_memory); // TODO password
        ASSERT_EQ(helperZipEntry(zipper, "somefile", "helloworld", "Test1"), true);
        zipper.close();

        zipper::Unzipper unzipper(zip_in_memory);
        ASSERT_EQ(unzipper.extractEntry("Test1"), true);
        unzipper.close();

        ASSERT_EQ(Path::exist("Test1"), true);
        ASSERT_EQ(Path::isFile("Test1"), true);
        ASSERT_STREQ(readFileContent("Test1").c_str(), "helloworld");

        Path::remove("Test1");
    }

    Path::remove("Test1");
}

// -----------------------------------------------------------------------------
TEST(ZipTests, ZipDifferentCompression)
{
    Path::remove("Test1");
    Path::remove("ziptest.zip");

    {
        ASSERT_EQ(createFile("Test1", "hello world"), true);
        std::ifstream ifs("Test1");

        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add(ifs, "Test1", Zipper::ZipFlags::Store), true);

        std::ifstream ifs1("Test1"); //FIXME
        ASSERT_EQ(zipper.add(ifs1, "Test1_1", Zipper::ZipFlags::Faster), true); // TODO zipper.add("Test1", "Test1_1" ...
        ASSERT_EQ(zipper.add(ifs, "Test1_2", Zipper::ZipFlags::Medium), true);
        ASSERT_EQ(zipper.add(ifs, "Test1_3", Zipper::ZipFlags::Better), true);
        zipper.close();

        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.extractAll(), true);
        unzipper.close();

        ASSERT_EQ(Path::exist("Test1"), true);
        ASSERT_EQ(Path::isFile("Test1"), true);
        ASSERT_STREQ(readFileContent("Test1").c_str(), "hello world");
        Path::remove("Test1");

        ASSERT_EQ(Path::exist("Test1_1"), true);
        ASSERT_EQ(Path::isFile("Test1_1"), true);
        ASSERT_STREQ(readFileContent("Test1_1").c_str(), "hello world");
        Path::remove("Test1_1");

        ASSERT_EQ(Path::exist("Test1_2"), true);
        ASSERT_EQ(Path::isFile("Test1_2"), true);
        ASSERT_STREQ(readFileContent("Test1_2").c_str(), "hello world");
        Path::remove("Test1_2");

        ASSERT_EQ(Path::exist("Test1_3"), true);
        ASSERT_EQ(Path::isFile("Test1_3"), true);
        ASSERT_STREQ(readFileContent("Test1_3").c_str(), "hello world");
        Path::remove("Test1_2");

        Path::remove("ziptest.zip");
    }

    // TODO password
}

// -----------------------------------------------------------------------------
TEST(ZipTests, ZipUtf8FileName)
{
}

// -----------------------------------------------------------------------------
TEST(ZipTests, ZipVeryLongFileName) // and fuzzing zip name and entries
{
}
#endif

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/21
TEST(ZipTests, Issue21)
{
    // Clean up
    Path::remove("ziptest.zip");
    Path::remove("data");

    // Create folder
    Path::createDir("data/somefolder/");
    std::ofstream test("data/somefolder/test.txt");
    test << "test file2 compression";
    test.flush();
    test.close();

    // Test with the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder/", Zipper::SaveHierarchy),
                  true);                                 // With the '/'
        ASSERT_EQ(zipper.add("data/somefolder/"), true); // With the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 2u);
        EXPECT_EQ(unzipper.entries()[0].name, "data/somefolder/test.txt");
        EXPECT_EQ(unzipper.entries()[1].name, "test.txt");
        Path::remove("ziptest.zip");
    }

    // Test without the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder", Zipper::SaveHierarchy),
                  true);                                // Without the '/'
        ASSERT_EQ(zipper.add("data/somefolder"), true); // Without the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 2u);
        EXPECT_EQ(unzipper.entries()[0].name, "data/somefolder/test.txt");
        EXPECT_EQ(unzipper.entries()[1].name, "test.txt");
    }

    Path::remove("ziptest.zip");
    Path::remove("data");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/33
TEST(ZipTests, Issue33_zipping)
{
    {
        Path::remove("ziptest.zip");
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(helperZipEntry(zipper, "Test1.txt", "hello", "../Test1"),
                  false);
        ASSERT_STREQ(
            zipper.error().message().c_str(),
            "Security error: forbidden insertion of '../Test1' "
            "(canonical: '../Test1') to prevent possible Zip Slip attack");
        zipper.close();

        Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 0u);
        unzipper.close();
    }

    {
        Path::remove("ziptest.zip");
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(helperZipEntry(zipper, "Test1.txt", "world", "foo/../Test1"),
                  true);
        zipper.close();

        Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "Test1");
        unzipper.close();
    }

    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/33
TEST(ZipTests, Issue33_unzipping)
{
    {
        ASSERT_EQ(Path::exist("../Test1"), false);

        Unzipper unzipper(PWD "/issues/issue33_1.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "../Test1");
        ASSERT_EQ(unzipper.extractEntry("../Test1"), false);
#if defined(_WIN32)
        ASSERT_STREQ(unzipper.error().message().c_str(),
                     "Security error: entry '..\\Test1' would be outside your "
                     "target directory");
#else
        ASSERT_STREQ(unzipper.error().message().c_str(),
                     "Security error: entry '../Test1' would be outside your "
                     "target directory");
#endif

        unzipper.close();
        ASSERT_EQ(Path::exist("../Test1"), false);
    }

    {
        Path::remove("Test1");

        Unzipper unzipper(PWD "/issues/issue33_2.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "foo/../Test1");
        ASSERT_EQ(unzipper.extractEntry("foo/../Test1"), true);
        unzipper.close();
        ASSERT_EQ(Path::exist("Test1"), true);
        ASSERT_EQ(Path::isFile("Test1"), true);
        ASSERT_STREQ(readFileContent("Test1").c_str(), "hello");

        Path::remove("Test1");
    }
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/34
TEST(ZipTests, Issue34)
{
    zipper::Unzipper unzipper(PWD "/issues/issue34.zip");
    ASSERT_EQ(unzipper.entries().size(), 13u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "issue34/");
    ASSERT_STREQ(unzipper.entries()[1].name.c_str(), "issue34/1/");
    ASSERT_STREQ(unzipper.entries()[2].name.c_str(), "issue34/1/.dummy");
    ASSERT_STREQ(unzipper.entries()[3].name.c_str(), "issue34/1/2/");
    ASSERT_STREQ(unzipper.entries()[4].name.c_str(), "issue34/1/2/3/");
    ASSERT_STREQ(unzipper.entries()[5].name.c_str(), "issue34/1/2/3/4/");
    ASSERT_STREQ(unzipper.entries()[6].name.c_str(), "issue34/1/2/3_1/");
    ASSERT_STREQ(unzipper.entries()[7].name.c_str(), "issue34/1/2/3_1/3.1.txt");
    ASSERT_STREQ(unzipper.entries()[8].name.c_str(), "issue34/1/2/foobar.txt");
    ASSERT_STREQ(unzipper.entries()[9].name.c_str(), "issue34/11/");
    ASSERT_STREQ(unzipper.entries()[10].name.c_str(), "issue34/11/foo/");
    ASSERT_STREQ(unzipper.entries()[11].name.c_str(), "issue34/11/foo/bar/");
    ASSERT_STREQ(unzipper.entries()[12].name.c_str(),
                 "issue34/11/foo/bar/here.txt");

    Path::remove(Path::getTempDirectory() + "issue34");
    ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory()), true);

    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/.dummy"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/2/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/2/3/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/2/3/4/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/2/3_1/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/2/3_1/3.1.txt"),
              true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/1/2/foobar.txt"),
              true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/11/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/11/foo/"), true);
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue34/11/foo/bar/"),
              true);
    ASSERT_EQ(
        Path::exist(Path::getTempDirectory() + "issue34/11/foo/bar/here.txt"),
        true);

    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/"), true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/1/"), true);
    ASSERT_EQ(Path::isFile(Path::getTempDirectory() + "issue34/1/.dummy"),
              true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/1/2/"), true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/1/2/3/"), true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/1/2/3/4/"), true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/1/2/3_1/"), true);
    ASSERT_EQ(
        Path::isFile(Path::getTempDirectory() + "issue34/1/2/3_1/3.1.txt"),
        true);
    ASSERT_EQ(Path::isFile(Path::getTempDirectory() + "issue34/1/2/foobar.txt"),
              true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/11/"), true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/11/foo/"), true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue34/11/foo/bar/"),
              true);
    ASSERT_EQ(
        Path::isFile(Path::getTempDirectory() + "issue34/11/foo/bar/here.txt"),
        true);

    std::string f1 = Path::getTempDirectory() + "issue34/1/2/3_1/3.1.txt";
    ASSERT_STREQ(readFileContent(f1.c_str()).c_str(), "3.1\n");
    std::string f2 = Path::getTempDirectory() + "issue34/1/2/foobar.txt";
    ASSERT_STREQ(readFileContent(f2.c_str()).c_str(), "foobar.txt\n");
    std::string f3 = Path::getTempDirectory() + "issue34/11/foo/bar/here.txt";
    ASSERT_STREQ(readFileContent(f3.c_str()).c_str(), "");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/83
TEST(MemoryZipTests, Issue83)
{
    // Clean up
    Path::remove("ziptest.zip");
    Path::remove("data");

    // Create folder
    Path::createDir("data/somefolder/");
    std::ofstream test("data/somefolder/test.txt");
    test << "test file2 compression";
    test.flush();
    test.close();

    // Zip
    Zipper zipper("ziptest.zip");
    ASSERT_EQ(zipper.add("data/somefolder/", Zipper::SaveHierarchy), true);
    zipper.close();

    // Unzip
    zipper::Unzipper unzipper("ziptest.zip");
    ASSERT_EQ(
        unzipper.extractEntry("data/somefolder/test.txt", "/does/not/exist"),
        false);
    ASSERT_STREQ(
        unzipper.error().message().c_str(),
        "Error: cannot create the folder '/does/not/exist/data/somefolder'");

    ASSERT_EQ(unzipper.extractEntry("data/somefolder/test.txt", "/usr/bin"),
              false);
    ASSERT_STREQ(unzipper.error().message().c_str(),
                 "Error: cannot create the folder '/usr/bin/data/somefolder'");

    ASSERT_EQ(unzipper.extractAll("/does/not/exist/"), false);
    ASSERT_STREQ(
        unzipper.error().message().c_str(),
        "Error: cannot create the folder '/does/not/exist/data/somefolder'");

    ASSERT_EQ(unzipper.extractAll("/usr/bin"), false);
    ASSERT_STREQ(unzipper.error().message().c_str(),
                 "Error: cannot create the folder '/usr/bin/data/somefolder'");

    Path::remove("ziptest.zip");
    Path::remove("data");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/118
TEST(MemoryZipTests, Issue118)
{
    Path::remove("ziptest.zip");

    // Create a dummy zip file
    Zipper zipper("ziptest.zip", Zipper::OpenFlags::Overwrite);
    zipper.close();

    // Check there is no entries
    zipper::Unzipper unzipper("ziptest.zip");
    std::vector<zipper::ZipEntry> entries = unzipper.entries();
    unzipper.close();
    ASSERT_EQ(entries.size(), 0u);

    // Add file from the dummy zip
    Zipper zipper2("ziptest.zip", Zipper::OpenFlags::Append);
    ASSERT_EQ(helperZipEntry(
                  zipper2, "test1.txt", "test1 file compression", "test1.txt"),
              true);
    zipper2.close();

    zipper::Unzipper unzipper2("ziptest.zip");
    std::vector<zipper::ZipEntry> entries2 = unzipper2.entries();
    unzipper2.close();
    ASSERT_EQ(entries2.size(), 1u);
    ASSERT_STREQ(entries2[0].name.c_str(), "test1.txt");
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ExtractFileWithNameOfDir)
{
    Path::remove("ziptest.zip");
    Zipper zipper("ziptest.zip");
    ASSERT_EQ(helperZipEntry(
                  zipper, "test1.txt", "test1 file compression", "test1.txt"),
              true);
    zipper.close();

    // Create a folder with a file name
    Path::remove(Path::getTempDirectory() + "foo/test1.txt");
    Path::createDir(Path::getTempDirectory() + "foo/test1.txt");
    ASSERT_EQ(Path::exist(Path::getTempDirectory() + "foo/test1.txt"), true);
    ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "foo/test1.txt"), true);

    // Check cannot extract the file because the folder with the same name
    // exists
    zipper::Unzipper unzipper("ziptest.zip");
    ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory() + "foo", false),
              false);
    std::string error =
        "Security Error: '" +
        Path::toNativeSeparators(Path::getTempDirectory() + "foo/test1.txt") +
        "' already exists and would have been replaced!";
    ASSERT_STREQ(unzipper.error().message().c_str(), error.c_str());

    // Check cannot extract the file because the folder with the same name
    // exists
    ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory() + "foo", true),
              false);
    error =
        "Failed creating '" +
        Path::toNativeSeparators(Path::getTempDirectory() + "foo/test1.txt") +
        "' file because Is a directory";
    ASSERT_STREQ(unzipper.error().message().c_str(), error.c_str());

    Path::remove(Path::getTempDirectory() + "foo/test1.txt");
}

// -----------------------------------------------------------------------------
// https://github.com/Lecrapouille/zipper/issues/5
TEST(MemoryZipTests, Issue5)
{
    // Zip given in the ticket
    {
        zipper::Unzipper unzipper(PWD "/issues/issue_05_1.zip");
        std::vector<zipper::ZipEntry> entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 3u);
        ASSERT_STREQ(entries[0].name.c_str(), "sim.sedml");
        ASSERT_STREQ(entries[1].name.c_str(), "model.xml");
        ASSERT_STREQ(entries[2].name.c_str(), "manifest.xml");

        ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory(), true), true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "sim.sedml"), true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "model.xml"), true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "manifest.xml"), true);
        ASSERT_EQ(Path::isFile(Path::getTempDirectory() + "sim.sedml"), true);
        ASSERT_EQ(Path::isFile(Path::getTempDirectory() + "model.xml"), true);
        ASSERT_EQ(Path::isFile(Path::getTempDirectory() + "manifest.xml"),
                  true);

        // Check cannot be extracted, by security: files already exist
        ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory()), false);
        std::string error = "Security Error: '" +
                            Path::toNativeSeparators(Path::getTempDirectory() +
                                                     "manifest.xml") +
                            "' already exists and would have been replaced!";
        ASSERT_STREQ(unzipper.error().message().c_str(), error.c_str());

        // Check cannot be extracted, by security: files already exist
        ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory(), false), false);
        ASSERT_STREQ(unzipper.error().message().c_str(), error.c_str());

        // Check can be extracted, by security: files already exist
        ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory(), true), true);

        unzipper.close();
    }

    // No password
    {
        zipper::Unzipper unzipper(PWD "/issues/issue_05_nopassword.zip");
        std::vector<zipper::ZipEntry> entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 5u);
        ASSERT_STREQ(entries[0].name.c_str(), "issue_05/");
        ASSERT_STREQ(entries[1].name.c_str(), "issue_05/Nouveau dossier/");
        ASSERT_STREQ(entries[2].name.c_str(), "issue_05/Nouveau fichier vide");
        ASSERT_STREQ(entries[3].name.c_str(), "issue_05/foo/");
        ASSERT_STREQ(entries[4].name.c_str(), "issue_05/foo/bar");

        Path::remove(Path::getTempDirectory() + "issue_05");
        ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory()), true);
        unzipper.close();

        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue_05/"), true);
        ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue_05/"), true);
        ASSERT_EQ(
            Path::exist(Path::getTempDirectory() + "issue_05/Nouveau dossier/"),
            true);
        ASSERT_EQ(
            Path::isDir(Path::getTempDirectory() + "issue_05/Nouveau dossier/"),
            true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() +
                              "issue_05/Nouveau fichier vide"),
                  true);
        ASSERT_EQ(Path::isFile(Path::getTempDirectory() +
                               "issue_05/Nouveau fichier vide"),
                  true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue_05/foo/"),
                  true);
        ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue_05/foo/"),
                  true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue_05/foo/bar"),
                  true);
        ASSERT_EQ(Path::isFile(Path::getTempDirectory() + "issue_05/foo/bar"),
                  true);

        Path::remove(Path::getTempDirectory() + "issue_05");
    }

    // With password
    {
        zipper::Unzipper unzipper(PWD "/issues/issue_05_password.zip", "1234");
        std::vector<zipper::ZipEntry> entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 5u);
        ASSERT_STREQ(entries[0].name.c_str(), "issue_05/");
        ASSERT_STREQ(entries[1].name.c_str(), "issue_05/Nouveau dossier/");
        ASSERT_STREQ(entries[2].name.c_str(), "issue_05/Nouveau fichier vide");
        ASSERT_STREQ(entries[3].name.c_str(), "issue_05/foo/");
        ASSERT_STREQ(entries[4].name.c_str(), "issue_05/foo/bar");

        Path::remove(Path::getTempDirectory() + "issue_05");
        ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory()), true);
        unzipper.close();

        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue_05/"), true);
        ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue_05/"), true);
        ASSERT_EQ(
            Path::exist(Path::getTempDirectory() + "issue_05/Nouveau dossier/"),
            true);
        ASSERT_EQ(
            Path::isDir(Path::getTempDirectory() + "issue_05/Nouveau dossier/"),
            true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() +
                              "issue_05/Nouveau fichier vide"),
                  true);
        ASSERT_EQ(Path::isFile(Path::getTempDirectory() +
                               "issue_05/Nouveau fichier vide"),
                  true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue_05/foo/"),
                  true);
        ASSERT_EQ(Path::isDir(Path::getTempDirectory() + "issue_05/foo/"),
                  true);
        ASSERT_EQ(Path::exist(Path::getTempDirectory() + "issue_05/foo/bar"),
                  true);
        ASSERT_EQ(Path::isFile(Path::getTempDirectory() + "issue_05/foo/bar"),
                  true);

        Path::remove(Path::getTempDirectory() + "issue_05");
    }
}

// -----------------------------------------------------------------------------
// Follow https://github.com/Lecrapouille/zipper/issues/5 try inserting files
// with folder extension
TEST(ZipTests, FileFakingFolder)
{
    Path::remove("ziptest.zip");
    Path::remove(Path::getTempDirectory() + "test");
    Path::remove(Path::getTempDirectory() + "test2");

    // Try adding test1.txt with folder extension
    Zipper zipper("ziptest.zip");
    ASSERT_EQ(helperZipEntry(
                  zipper, "test1.txt", "test1 file compression", "test1.txt/"),
              true);
    ASSERT_EQ(helperZipEntry(
                  zipper, "test2.txt", "test2 file compression", "test2.txt\\"),
              true);
    ASSERT_EQ(
        helperZipEntry(zipper, "test3.txt", "test3 file compression", "test\\"),
        true);
    ASSERT_EQ(helperZipEntry(
                  zipper, "test4.txt", "test4 file compression", "test2\\bar"),
              true);
    zipper.close();

    // Check files exist without folder extension
    zipper::Unzipper unzipper("ziptest.zip");
    std::vector<zipper::ZipEntry> entries = unzipper.entries();

    ASSERT_EQ(entries.size(), 4u);
    ASSERT_STREQ(entries[0].name.c_str(), "test1.txt");
    ASSERT_STREQ(entries[1].name.c_str(), "test2.txt");
    ASSERT_STREQ(entries[2].name.c_str(), "test");
    ASSERT_STREQ(entries[3].name.c_str(), "test2/bar");

    // Extract
    ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory(), true), true);
    std::string test1 =
        readFileContent((Path::getTempDirectory() + "test1.txt").c_str());
    ASSERT_STREQ(test1.c_str(), "test1 file compression");
    std::string test2 =
        readFileContent((Path::getTempDirectory() + "test2.txt").c_str());
    ASSERT_STREQ(test2.c_str(), "test2 file compression");
    std::string test3 =
        readFileContent((Path::getTempDirectory() + "test").c_str());
    ASSERT_STREQ(test3.c_str(), "test3 file compression");
    std::string test4 =
        readFileContent((Path::getTempDirectory() + "test2/bar").c_str());
    ASSERT_STREQ(test4.c_str(), "test4 file compression");

    unzipper.close();
    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
// Close unzipper and try accessing to it
TEST(ZipTests, UnzipperClosed)
{
    Path::remove("ziptest.zip");
    Zipper zipper("ziptest.zip");
    ASSERT_EQ(helperZipEntry(
                  zipper, "test1.txt", "test1 file compression", "test1.txt"),
              true);
    zipper.close();

    zipper::Unzipper unzipper("ziptest.zip");
    unzipper.close();

    std::vector<zipper::ZipEntry> entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 0u);

    ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory(), false), false);
    ASSERT_EQ(unzipper.extractAll(Path::getTempDirectory(), true), false);

    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
/**
 * @brief Test adding a large file with a password to trigger chunked CRC
 * calculation.
 */
TEST(ZipTests, LargeFileWithPassword)
{
    const std::string zipFilename = "ziptest_large.zip";
    const std::string largeFilename = "large_test_file.dat";
    const std::string password = "verysecretpassword";
    const size_t fileSize =
        110 * 1024 * 1024; // 110 MB to trigger chunked reading in getFileCrc

    // Clean up potential leftovers
    Path::remove(zipFilename);
    Path::remove(largeFilename);

    // Create a large file
    {
        std::ofstream ofs(largeFilename, std::ios::binary | std::ios::out);
        ASSERT_TRUE(ofs.is_open());
        // Fill with some data, 'A' for simplicity
        std::vector<char> buffer(1024 * 1024, 'A'); // 1MB buffer
        size_t bytesWritten = 0;
        while (bytesWritten < fileSize)
        {
            size_t toWrite = std::min(buffer.size(), fileSize - bytesWritten);
            ofs.write(buffer.data(), std::streamsize(toWrite));
            ASSERT_TRUE(ofs.good());
            bytesWritten += toWrite;
        }
        ofs.flush();
        ASSERT_TRUE(ofs.good());
        ofs.close();
    }
    ASSERT_TRUE(Path::exist(largeFilename));
    // Check file size using Path::getFileSize
    ASSERT_EQ(Path::getFileSize(largeFilename), fileSize);

    // Create zipper with password and add the large file
    {
        Zipper zipper(zipFilename, password);
        ASSERT_TRUE(zipper.isOpen()); // Use the new method
        std::ifstream ifs(largeFilename, std::ios::binary);
        ASSERT_TRUE(ifs.is_open());
        ASSERT_TRUE(zipper.add(ifs, largeFilename));
        ifs.close();
        zipper.close();
        ASSERT_FALSE(zipper.isOpen()); // Use the new method
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
    }

    // Verify the zip file
    ASSERT_TRUE(Path::exist(zipFilename));
    {
        zipper::Unzipper unzipper(zipFilename, password);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        std::vector<zipper::ZipEntry> entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1u);
        ASSERT_STREQ(entries[0].name.c_str(), largeFilename.c_str());
        ASSERT_EQ(entries[0].uncompressedSize, fileSize); // Check original size
        // We could extract and verify content, but for coverage, checking
        // existence is enough
        unzipper.close();
    }

    // Clean up
    Path::remove(zipFilename);
    Path::remove(largeFilename);
}

// -----------------------------------------------------------------------------
/**
 * @brief Test extracting a single entry to an std::ostream.
 */
TEST(UnzipTests, ExtractEntryToStream)
{
    const std::string entryName = "stream_entry.txt";
    const std::string entryContent = "hello stream test";
    std::stringstream zipDataStream; // To hold the zip data in memory

    // 1. Create zip in memory
    {
        zipper::Zipper zipper(zipDataStream);
        std::stringstream contentStream(entryContent);
        ASSERT_TRUE(zipper.add(contentStream, entryName));
        zipper.close();
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
    }

    // Ensure zipDataStream has content and reset position
    zipDataStream.seekg(0, std::ios::end);
    ASSERT_GT(zipDataStream.tellg(), 0);
    zipDataStream.seekg(0, std::ios::beg);

    // 2. Create Unzipper from the stream
    zipper::Unzipper unzipper(zipDataStream);
    ASSERT_FALSE(unzipper.error()) << unzipper.error().message();

    // 3. Create output stream
    std::stringstream outputStream;

    // 4. Extract the entry to the output stream
    ASSERT_TRUE(unzipper.extractEntryToStream(entryName, outputStream));
    ASSERT_FALSE(unzipper.error()) << unzipper.error().message();

    // 5. Verify the extracted content
    ASSERT_STREQ(outputStream.str().c_str(), entryContent.c_str());

    unzipper.close();
}