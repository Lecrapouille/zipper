#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "utils/Path.hpp" // May be needed for entry names
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Hack - Copy for now, remove later
#define protected public
#define private public
#include "Zipper/Zipper.hpp"
// Need Unzipper to verify memory content
#include "Zipper/Unzipper.hpp"
#undef protected
#undef private

using namespace zipper;

// --- Helpers (if needed, e.g., for creating input streams) ---
static bool createFile(const std::string& file, const std::string& content)
{
    std::ofstream ofs(file);
    if (!ofs)
        return false;
    ofs << content;
    ofs.flush();
    ofs.close();
    return Path::exist(file) && Path::isFile(file);
}

TEST(ZipperMemoryOps, ZipToVector)
{
    std::vector<unsigned char> zipData;
    const std::string entryName = "vector_entry.txt";
    const std::string content = "vector content";

    {
        Zipper zipper(zipData); // Constructor for vector
        std::stringstream contentStream(content);
        ASSERT_TRUE(zipper.add(contentStream, entryName));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close(); // Vector is updated on close
    }

    ASSERT_FALSE(zipData.empty()); // Check vector is not empty

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
}

TEST(ZipperMemoryOps, ZipMultipleToVector)
{
    std::vector<unsigned char> zipData;
    const std::string entryName1 = "multi_vec1.txt";
    const std::string content1 = "multi vec content 1";
    const std::string entryName2 = "folder/multi_vec2.dat";
    const std::string content2 = "multi vec content 2";

    {
        Zipper zipper(zipData);
        std::stringstream stream1(content1);
        std::stringstream stream2(content2);
        ASSERT_TRUE(zipper.add(stream1, entryName1));
        ASSERT_TRUE(zipper.add(stream2, entryName2));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }

    ASSERT_FALSE(zipData.empty());

    // Verify
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

TEST(ZipperMemoryOps, ZipWithPasswordToVector)
{
    std::vector<unsigned char> zipData;
    const std::string entryName = "pwd_vec.txt";
    const std::string content = "pwd vec content";
    const std::string password = "memory_password";

    {
        Zipper zipper(zipData, password); // Use password constructor
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

TEST(ZipperMemoryOps, ZipFromExternalFileToVector)
{
    // Test adding a file from disk when zipper target is memory
    std::vector<unsigned char> zipData;
    const std::string tempFileName = "temp_file_for_memzip.txt";
    const std::string content = "disk file to memory zip";
    const std::string entryName = "disk_file_entry.txt";

    ASSERT_TRUE(createFile(tempFileName, content));

    {
        Zipper zipper(zipData);
        std::ifstream ifs(tempFileName,
                          std::ios::binary); // Open the file stream
        ASSERT_TRUE(ifs.is_open())
            << "Failed to open temporary file: " << tempFileName;
        // Add using the stream and the desired entry name
        ASSERT_TRUE(zipper.add(ifs, entryName)); // Use specific name in zip
        ifs.close();                             // Close the stream
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close();
    }
    Path::remove(tempFileName);
    ASSERT_FALSE(zipData.empty());

    // Verify
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

#if 0
TEST(ZipperMemoryOps, ZipToStream)
{
    const std::string entryName = "stream_entry.txt";
    const std::string content = "stream content";
    std::stringstream zipDataStream;

    {
        Zipper zipper(zipDataStream); // Constructor for stream
        std::ifstream input1(entryName);
        ASSERT_TRUE(zipper.add(input1, content));
        ASSERT_FALSE(zipper.error()) << zipper.error().message();
        zipper.close(); // Stream is updated on close
    }

    // Check stream content exists
    zipDataStream.seekg(0, std::ios::end);
    ASSERT_GT(zipDataStream.tellg(), 0);
    zipDataStream.seekg(0, std::ios::beg); // Reset for unzipper

    // Verify content using Unzipper
    {
        Unzipper unzipper(zipDataStream);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1);
        ASSERT_EQ(entries[0].name, entryName);

        std::stringstream extractedStream;
        ASSERT_TRUE(unzipper.extractEntryToStream(entryName, extractedStream));
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        ASSERT_EQ(extractedStream.str(), content);
        unzipper.close();
    }
}
#endif