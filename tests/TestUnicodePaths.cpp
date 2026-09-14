#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Zipper/Unzipper.hpp"
#include "Zipper/Zipper.hpp"
#include "TestHelper.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace zipper;

namespace
{

namespace fs = std::filesystem;

fs::path u8path(const char8_t* p_text)
{
    return fs::path(std::u8string(p_text));
}

std::string readAll(const fs::path& p_file)
{
    std::ifstream ifs(p_file, std::ios::binary);
    return { std::istreambuf_iterator<char>(ifs),
             std::istreambuf_iterator<char>() };
}

class UnicodePathsEnv: public ::testing::Test
{
protected:

    void SetUp() override
    {
        m_root = fs::temp_directory_path() / u8path(u8"zipper_unicode_тест_café");
        fs::remove_all(m_root);
        fs::create_directories(m_root);
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_root, ec);
    }

    fs::path m_root;
};

} // namespace

//-----------------------------------------------------------------------------
TEST_F(UnicodePathsEnv, AsciiPathsStillWork)
{
    const fs::path zip_path = m_root / "ascii_regression.zip";
    const fs::path src = m_root / "plain.txt";
    {
        std::ofstream ofs(src, std::ios::binary);
        ofs << "ascii-ok";
    }

    {
        Zipper zipper(helper::utf8(zip_path));
        ASSERT_TRUE(zipper.isOpened()) << zipper.error().message();
        ASSERT_TRUE(zipper.add(helper::utf8(src))) << zipper.error().message();
        zipper.close();
    }

    ASSERT_TRUE(fs::exists(zip_path));

    const fs::path out = m_root / "ascii_out";
    {
        Unzipper unzipper(helper::utf8(zip_path));
        ASSERT_TRUE(unzipper.isOpened()) << unzipper.error().message();
        ASSERT_TRUE(unzipper.extractAll(helper::utf8(out),
                                       Unzipper::OverwriteMode::Overwrite))
            << unzipper.error().message();
    }

    ASSERT_EQ(readAll(out / "plain.txt"), "ascii-ok");
}

//-----------------------------------------------------------------------------
TEST_F(UnicodePathsEnv, ZipAddExtractUnicodeFilesystemPath)
{
    const fs::path zip_path = m_root / u8path(u8"архив_café.zip");
    const fs::path src_dir = m_root / u8path(u8"src_données");
    const fs::path src_file = src_dir / u8path(u8"naïve_日本語.txt");
    const fs::path extract_dir = m_root / u8path(u8"extrait");
    const std::string payload = "hello unicode";

    fs::create_directories(src_dir);
    {
        std::ofstream ofs(src_file, std::ios::binary);
        ASSERT_TRUE(ofs.good());
        ofs << payload;
    }

    {
        Zipper zipper(zip_path);
        ASSERT_TRUE(zipper.isOpened()) << zipper.error().message();
        ASSERT_TRUE(zipper.add(src_file)) << zipper.error().message();
        zipper.close();
    }

    ASSERT_TRUE(fs::exists(zip_path));

    {
        Unzipper unzipper(zip_path);
        ASSERT_TRUE(unzipper.isOpened()) << unzipper.error().message();
        const auto entries = unzipper.entries();
        ASSERT_EQ(entries.size(), 1u);
        EXPECT_NE(entries.front().name.find(helper::utf8(u8"naïve_日本語.txt")),
                  std::string::npos);
        ASSERT_TRUE(unzipper.extractAll(extract_dir,
                                       Unzipper::OverwriteMode::Overwrite))
            << unzipper.error().message();
    }

    bool found = false;
    for (const auto& entry : fs::recursive_directory_iterator(extract_dir))
    {
        if (entry.is_regular_file() && readAll(entry.path()) == payload)
        {
            found = true;
        }
    }
    ASSERT_TRUE(found);
}

//-----------------------------------------------------------------------------
TEST_F(UnicodePathsEnv, Utf8StringPaths)
{
    const fs::path zip_path = m_root / u8path(u8"utf8_string.zip");
    const fs::path src = m_root / u8path(u8"été.txt");
    {
        std::ofstream ofs(src, std::ios::binary);
        ofs << helper::utf8(u8"été");
    }

    const auto zip_utf8 = helper::utf8(zip_path);
    const auto src_utf8 = helper::utf8(src);

    {
        Zipper zipper(zip_utf8);
        ASSERT_TRUE(zipper.isOpened()) << zipper.error().message();
        ASSERT_TRUE(zipper.add(src_utf8)) << zipper.error().message();
        zipper.close();
    }

    {
        Unzipper unzipper(zip_utf8);
        ASSERT_TRUE(unzipper.isOpened()) << unzipper.error().message();
        std::vector<unsigned char> out;
        ASSERT_TRUE(unzipper.extract(helper::utf8(u8"été.txt"), out))
            << unzipper.error().message();
        const std::string extracted(out.begin(), out.end());
        ASSERT_EQ(extracted, helper::utf8(u8"été"));
    }
}

//-----------------------------------------------------------------------------
TEST_F(UnicodePathsEnv, OpenSwitchBetweenPathAndUtf8AndAppend)
{
    const fs::path zip_path = m_root / u8path(u8"reopen_данные.zip");

    Zipper zipper;
    ASSERT_TRUE(zipper.open(zip_path)) << zipper.error().message();
    {
        std::stringstream first("one");
        ASSERT_TRUE(zipper.add(first, "one.txt"));
    }
    zipper.close();

    const auto zip_utf8 = helper::utf8(zip_path);

    ASSERT_TRUE(zipper.open(zip_utf8, std::string(), Zipper::OpenFlags::Append))
        << zipper.error().message();
    {
        std::stringstream second("two");
        ASSERT_TRUE(zipper.add(second, "two.txt"));
    }
    zipper.close();

    ASSERT_TRUE(zipper.open(zip_path, Zipper::OpenFlags::Append))
        << zipper.error().message();
    {
        std::stringstream third("three");
        ASSERT_TRUE(zipper.add(third, "three.txt"));
    }
    zipper.close();

    Unzipper unzipper(zip_path);
    ASSERT_TRUE(unzipper.isOpened()) << unzipper.error().message();
    ASSERT_EQ(unzipper.entries().size(), 3u);
}

//-----------------------------------------------------------------------------
TEST_F(UnicodePathsEnv, AddUnicodeFolderWithHierarchy)
{
    const fs::path zip_path = m_root / u8path(u8"folder.zip");
    const fs::path folder = m_root / u8path(u8"dossier_été");
    const fs::path nested = folder / u8path(u8"sous");
    fs::create_directories(nested);
    {
        std::ofstream ofs(nested / u8path(u8"fichier.txt"), std::ios::binary);
        ofs << "nested";
    }

    {
        Zipper zipper(zip_path);
        ASSERT_TRUE(zipper.add(folder, Zipper::SaveHierarchy))
            << zipper.error().message();
        zipper.close();
    }

    Unzipper unzipper(zip_path);
    const auto entries = unzipper.entries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_NE(entries.front().name.find("fichier.txt"), std::string::npos);
    EXPECT_NE(entries.front().name.find(helper::utf8(u8"dossier_été")),
              std::string::npos);
}

#if defined(_WIN32)
//-----------------------------------------------------------------------------
TEST_F(UnicodePathsEnv, WindowsWideStringOverload)
{
    const fs::path zip_path = m_root / u8path(u8"wide_wstring.zip");
    {
        Zipper zipper(zip_path.wstring());
        ASSERT_TRUE(zipper.isOpened()) << zipper.error().message();
        std::stringstream ss("wide");
        ASSERT_TRUE(zipper.add(ss, "wide.txt"));
        zipper.close();
    }

    Unzipper unzipper(zip_path.wstring());
    ASSERT_TRUE(unzipper.isOpened()) << unzipper.error().message();
    std::vector<unsigned char> out;
    ASSERT_TRUE(unzipper.extract("wide.txt", out));
    ASSERT_EQ(std::string(out.begin(), out.end()), "wide");
}
#endif
