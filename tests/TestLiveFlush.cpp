//-----------------------------------------------------------------------------
// Tests for the "live" in-memory output: minizip now writes directly into the
// caller's std::vector / std::iostream, and flush()/auto-flush let the caller
// obtain a valid, parsable ZIP without closing the Zipper.
//-----------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "Zipper/Unzipper.hpp"
#include "Zipper/Zipper.hpp"
#include "TestHelper.hpp"

#include <sstream>

using namespace zipper;

//=============================================================================
// The referenced vector is written as entries are added (no longer only at
// close()): its size grows after each add(), before close().
//=============================================================================
TEST(LiveOutput, VectorGrowsWhileAddingBeforeClose)
{
    std::vector<unsigned char> zip_data;

    Zipper zipper(zip_data);
    ASSERT_TRUE(zip_data.empty());

    std::stringstream s1("first entry content");
    ASSERT_TRUE(zipper.add(s1, "a.txt"));
    const size_t after_first = zip_data.size();
    ASSERT_GT(after_first, 0u) << "vector must receive bytes as soon as an "
                                  "entry is added";

    std::stringstream s2("second entry content, a bit longer");
    ASSERT_TRUE(zipper.add(s2, "b.txt"));
    ASSERT_GT(zip_data.size(), after_first);

    zipper.close();
    ASSERT_FALSE(zip_data.empty());

    // Sanity: the closed archive is valid and holds both entries.
    Unzipper unzipper(zip_data);
    ASSERT_EQ(unzipper.entries().size(), 2u);
    unzipper.close();
}

//=============================================================================
// flush() turns the in-progress vector into a valid archive while keeping the
// Zipper open for more entries.
//=============================================================================
TEST(LiveOutput, FlushMakesVectorParsableAndKeepsZipperOpen)
{
    std::vector<unsigned char> zip_data;
    Zipper zipper(zip_data);

    std::stringstream s1("hello");
    ASSERT_TRUE(zipper.add(s1, "one.txt"));

    // Before flush the raw bytes are there but there is no central directory.
    ASSERT_TRUE(zipper.flush());
    ASSERT_FALSE(zipper.error()) << zipper.error().message();

    // The vector is now a valid ZIP that a fresh Unzipper can parse.
    {
        Unzipper unzipper(zip_data);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1u);
        EXPECT_EQ(entries[0].name, "one.txt");

        std::vector<unsigned char> out;
        ASSERT_TRUE(unzipper.extract("one.txt", out));
        EXPECT_EQ(std::string(out.begin(), out.end()), "hello");
        unzipper.close();
    }

    // The Zipper is still usable: add another entry and flush again.
    std::stringstream s2("world");
    ASSERT_TRUE(zipper.add(s2, "two.txt"));
    ASSERT_TRUE(zipper.flush());

    {
        Unzipper unzipper(zip_data);
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 2u);
        EXPECT_EQ(entries[0].name, "one.txt");
        EXPECT_EQ(entries[1].name, "two.txt");
        unzipper.close();
    }

    zipper.close();

    // Final archive still valid and complete.
    Unzipper unzipper(zip_data);
    ASSERT_EQ(unzipper.entries().size(), 2u);
    unzipper.close();
}

//=============================================================================
// flush() on a std::iostream backend.
//=============================================================================
TEST(LiveOutput, FlushOnStream)
{
    std::stringstream zip_stream;
    Zipper zipper(zip_stream, Zipper::OpenFlags::Overwrite);

    std::stringstream s1("stream payload");
    ASSERT_TRUE(zipper.add(s1, "s.txt"));
    ASSERT_TRUE(zipper.flush());

    // Copy the current stream content and parse it independently.
    const std::string snapshot = zip_stream.str();
    ASSERT_FALSE(snapshot.empty());

    std::vector<unsigned char> as_vec(snapshot.begin(), snapshot.end());
    Unzipper unzipper(as_vec);
    ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
    auto entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].name, "s.txt");
    unzipper.close();

    zipper.close();
}

//=============================================================================
// Auto-flush: after each add() the referenced vector is already a valid ZIP.
//=============================================================================
TEST(LiveOutput, AutoFlushKeepsVectorValidAfterEachAdd)
{
    std::vector<unsigned char> zip_data;
    Zipper zipper(zip_data);

    EXPECT_FALSE(zipper.autoFlush());
    zipper.setAutoFlush(true);
    EXPECT_TRUE(zipper.autoFlush());

    std::stringstream s1("alpha");
    ASSERT_TRUE(zipper.add(s1, "alpha.txt"));

    // No explicit flush()/close(): the vector is already parsable.
    {
        Unzipper unzipper(zip_data);
        ASSERT_FALSE(unzipper.error()) << unzipper.error().message();
        ASSERT_EQ(unzipper.entries().size(), 1u);
        unzipper.close();
    }

    std::stringstream s2("beta");
    ASSERT_TRUE(zipper.add(s2, "beta.txt"));

    {
        Unzipper unzipper(zip_data);
        auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 2u);
        EXPECT_EQ(entries[0].name, "alpha.txt");
        EXPECT_EQ(entries[1].name, "beta.txt");

        std::vector<unsigned char> out;
        ASSERT_TRUE(unzipper.extract("beta.txt", out));
        EXPECT_EQ(std::string(out.begin(), out.end()), "beta");
        unzipper.close();
    }

    zipper.close();

    Unzipper unzipper(zip_data);
    ASSERT_EQ(unzipper.entries().size(), 2u);
    unzipper.close();
}

//=============================================================================
// Auto-flush does not corrupt the archive across several small entries and the
// content stays correct.
//=============================================================================
TEST(LiveOutput, AutoFlushManyEntriesRoundTrip)
{
    std::vector<unsigned char> zip_data;
    Zipper zipper(zip_data);
    zipper.setAutoFlush(true);

    const int count = 12;
    for (int i = 0; i < count; ++i)
    {
        std::stringstream content;
        content << "content-" << i;
        std::stringstream name_stream;
        name_stream << "file_" << i << ".txt";
        ASSERT_TRUE(zipper.add(content, name_stream.str()))
            << "add failed at " << i << ": " << zipper.error().message();
    }
    zipper.close();

    Unzipper unzipper(zip_data);
    auto entries = unzipper.entries();
    ASSERT_EQ(entries.size(), static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        std::stringstream name_stream;
        name_stream << "file_" << i << ".txt";
        std::vector<unsigned char> out;
        ASSERT_TRUE(unzipper.extract(name_stream.str(), out));
        std::stringstream expected;
        expected << "content-" << i;
        EXPECT_EQ(std::string(out.begin(), out.end()), expected.str());
    }
    unzipper.close();
}

//=============================================================================
// flush() on a not-yet-opened Zipper reports an error rather than crashing.
//=============================================================================
TEST(LiveOutput, FlushOnClosedZipperFails)
{
    std::vector<unsigned char> zip_data;
    Zipper zipper(zip_data);
    std::stringstream s1("x");
    ASSERT_TRUE(zipper.add(s1, "x.txt"));
    zipper.close();

    EXPECT_FALSE(zipper.flush());
}
