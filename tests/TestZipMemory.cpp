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
// Tests for memory zip operations
//=============================================================================
TEST(ZipperMemoryOps, ZipToVector)
{
    std::vector<unsigned char> zipData;
    const std::string entryName = "vector_entry.txt";
    const std::string content = "vector content";

    // Constructor for vector
    {
        ASSERT_TRUE(zipData.empty());

        Zipper zipper(zipData);
        ASSERT_TRUE(zipData.empty());

        std::stringstream contentStream(content);
        ASSERT_TRUE(zipper.add(contentStream, entryName));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        ASSERT_TRUE(zipData.empty());

        zipper.close();
        ASSERT_FALSE(zipData.empty());
    }

    // Verify content using Unzipper
    {
        Unzipper unzipper(zipData);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1);
        ASSERT_EQ(entries[0].name, entryName);

        std::vector<unsigned char> extractedData;
        ASSERT_TRUE(unzipper.extractEntryToMemory(entryName, extractedData));
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        std::string extractedString(extractedData.begin(), extractedData.end());
        ASSERT_EQ(extractedString, content);
        unzipper.close();
    }

    // Open with the same non empty vector
    {
        Zipper zipper(zipData);
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();

        Unzipper unzipper(zipData);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1);
        ASSERT_EQ(entries[0].name, entryName);
        unzipper.close();
    }
}

//=============================================================================
// Tests for zip multiple entries to vector
//=============================================================================
TEST(ZipperMemoryOps, ZipMultipleToVector)
{
    std::vector<unsigned char> zipData;
    const std::string entryName1 = "multi_vec1.txt";
    const std::string content1 = "multi vec content 1";
    const std::string entryName2 = "folder/multi_vec2.dat";
    const std::string content2 = "multi vec content 2";

    // Constructor for vector
    {
        ASSERT_TRUE(zipData.empty());

        Zipper zipper(zipData);
        ASSERT_TRUE(zipData.empty());

        std::stringstream stream1(content1);
        std::stringstream stream2(content2);
        ASSERT_TRUE(zipper.add(stream1, entryName1));
        ASSERT_TRUE(zipper.add(stream2, entryName2));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        ASSERT_TRUE(zipData.empty());

        zipper.close();
        ASSERT_FALSE(zipData.empty());
    }

    // Verify content using Unzipper
    {
        Unzipper unzipper(zipData);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 2);

        std::vector<unsigned char> data1, data2;
        ASSERT_TRUE(unzipper.extractEntryToMemory(entryName1, data1));
        ASSERT_EQ(std::string(data1.begin(), data1.end()), content1);
        ASSERT_TRUE(unzipper.extractEntryToMemory(entryName2, data2));
        ASSERT_EQ(std::string(data2.begin(), data2.end()), content2);
        unzipper.close();
    }
}

//=============================================================================
// Tests for zip with password to vector
//=============================================================================
TEST(ZipperMemoryOps, ZipWithPasswordToVector)
{
    std::vector<unsigned char> zipData;
    const std::string entryName = "pwd_vec.txt";
    const std::string content = "pwd vec content";
    const std::string password = "memory_password";

    // Use password constructor
    {
        Zipper zipper(zipData, password);
        std::stringstream contentStream(content);
        ASSERT_TRUE(zipper.add(contentStream, entryName));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }
    ASSERT_FALSE(zipData.empty());

    // Verify with Unzipper (correct password)
    {
        Unzipper unzipper(zipData, password);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        std::vector<unsigned char> data;
        ASSERT_TRUE(unzipper.extractEntryToMemory(entryName, data));
        ASSERT_EQ(std::string(data.begin(), data.end()), content);
        unzipper.close();
    }

    // Verify with Unzipper (wrong password)
    {
        Unzipper unzipper(zipData, "wrong_pwd");
        ASSERT_FALSE(unzipper.error())
            << "Constructor should not fail on wrong password.";
        std::vector<unsigned char> data;
        ASSERT_FALSE(unzipper.extractEntryToMemory(
            entryName, data)); // Extraction should fail
        ASSERT_TRUE(unzipper.error());
        ASSERT_TRUE(data.empty());
        unzipper.close();
    }
}

//=============================================================================
// Tests for zip from external file to vector
//=============================================================================
TEST(ZipperMemoryOps, ZipFromExternalFileToVector)
{
    // Test adding a file from disk when zipper target is memory
    std::vector<unsigned char> zipData;
    const std::string tempFileName = "temp_file_for_memzip.txt";
    const std::string content = "disk file to memory zip";
    const std::string entryName = "disk_file_entry.txt";

    ASSERT_TRUE(helper::createFile(tempFileName, content));

    {
        Zipper zipper(zipData);
        std::ifstream ifs(tempFileName, std::ios::binary);
        ASSERT_TRUE(ifs.is_open())
            << "Failed to open temporary file: " << tempFileName;
        ASSERT_TRUE(zipper.add(ifs, entryName));
        ifs.close();
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    ASSERT_TRUE(helper::removeFileOrDir(tempFileName));
    ASSERT_FALSE(zipData.empty());

    // Verify content using Unzipper
    {
        Unzipper unzipper(zipData);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        ASSERT_EQ(unzipper.entries().size(), 1);
        ASSERT_EQ(unzipper.entries()[0].name, entryName);

        std::vector<unsigned char> data;
        ASSERT_TRUE(unzipper.extractEntryToMemory(entryName, data));
        ASSERT_EQ(std::string(data.begin(), data.end()), content);
        unzipper.close();
    }
}

//=============================================================================
// Tests for zip to stream
//=============================================================================
TEST(ZipperMemoryOps, ZipToStream)
{
    std::stringstream zipDataStream;
    const std::string entryName = "stream_entry.txt";
    const std::string content = "stream content";

    std::stringstream inputStream2("append content");
    const std::string appendEntryName = "append_entry.txt";

    // Constructor for stream
    {
        Zipper zipper(zipDataStream, Zipper::OpenFlags::Overwrite);
        std::stringstream inputStream(content);
        ASSERT_TRUE(zipper.add(inputStream, entryName));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Check stream content exists
    zipDataStream.seekg(0, std::ios::end);
    ASSERT_GT(zipDataStream.tellg(), 0);
    zipDataStream.seekg(0, std::ios::beg); // Reset for unzipper

    // Append to stream
    {
        Zipper zipper(zipDataStream, Zipper::OpenFlags::Append);
        ASSERT_TRUE(zipper.add(inputStream2, appendEntryName));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Reopen the stream
    {
        Zipper zipper;
        zipper.open(zipDataStream, "password", Zipper::OpenFlags::Append);
        ASSERT_TRUE(zipper.add(inputStream2, appendEntryName));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    // Verify content using Unzipper
    {
        Unzipper unzipper(zipDataStream);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 3);
        ASSERT_EQ(entries[0].name, entryName);
        ASSERT_EQ(entries[1].name, appendEntryName);
        ASSERT_EQ(entries[2].name, appendEntryName);

        std::stringstream extractedStream;
        ASSERT_TRUE(unzipper.extractEntryToStream(entryName, extractedStream));
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        ASSERT_EQ(extractedStream.str(), content);
        unzipper.close();
    }

    // Test with empty stream
    {
        Zipper zipper(zipDataStream, Zipper::OpenFlags::Overwrite);
        zipper.close();

        Unzipper unzipper(zipDataStream);
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 0);
        unzipper.close();
    }

    // Test open() with stream and password
    {
        Zipper zipper;
        ASSERT_TRUE(zipper.open(zipDataStream, "test_password"));
        std::stringstream inputStream("password protected content");
        ASSERT_TRUE(zipper.add(inputStream, "password_entry.txt"));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();

        // Verify with Unzipper using password
        Unzipper unzipper(zipDataStream, "test_password");
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1);
        ASSERT_EQ(entries[0].name, "password_entry.txt");
        unzipper.close();
    }

    // Test open() with stream, password and flags
    {
        Zipper zipper;
        ASSERT_TRUE(zipper.open(
            zipDataStream, "test_password2", Zipper::OpenFlags::Append));
        std::stringstream inputStream("another password protected content");
        ASSERT_TRUE(zipper.add(inputStream, "password_entry2.txt"));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();

        // Verify with Unzipper using password
        Unzipper unzipper(zipDataStream, "test_password2");
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 2); // Should have both password entries
        unzipper.close();
    }

    // Test open() with stream and flags (no password)
    {
        Zipper zipper;
        ASSERT_TRUE(
            zipper.open(zipDataStream, "", Zipper::OpenFlags::Overwrite));
        std::stringstream inputStream("content without password");
        ASSERT_TRUE(zipper.add(inputStream, "no_password_entry.txt"));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();

        // Verify with Unzipper (no password needed)
        Unzipper unzipper(zipDataStream);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1);
        ASSERT_EQ(entries[0].name, "no_password_entry.txt");
        unzipper.close();
    }
}