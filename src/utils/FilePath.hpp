//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef ZIPPER_UTILS_FILEPATH_HPP
#define ZIPPER_UTILS_FILEPATH_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#if defined(_WIN32)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <Windows.h>
#endif

namespace zipper
{

// -----------------------------------------------------------------------------
//! \brief Convert a native filesystem path to a UTF-8 std::string.
// -----------------------------------------------------------------------------
inline std::string pathToUtf8(const std::filesystem::path& p_path)
{
    const auto u8 = p_path.u8string();
    return { u8.begin(), u8.end() };
}

// -----------------------------------------------------------------------------
//! \brief Build a filesystem path from a UTF-8 string.
//!
//! On Windows this uses the UTF-8 path constructor so the result is a wide
//! native path, not the current ANSI code page.
// -----------------------------------------------------------------------------
inline std::filesystem::path utf8ToPath(std::string_view p_utf8)
{
    const auto* bytes = reinterpret_cast<const char8_t*>(p_utf8.data());
    return std::filesystem::path(std::u8string(bytes, bytes + p_utf8.size()));
}

inline std::filesystem::path utf8ToPath(const char* p_utf8)
{
    return utf8ToPath(std::string_view(p_utf8 != nullptr ? p_utf8 : ""));
}

inline std::filesystem::path utf8ToPath(const std::string& p_utf8)
{
    return utf8ToPath(std::string_view(p_utf8));
}

// -----------------------------------------------------------------------------
//! \brief Last OS error as a printable message (GetLastError on Windows).
// -----------------------------------------------------------------------------
inline std::string nativeErrorMessage()
{
#if defined(_WIN32)
    const DWORD code = GetLastError();
    if (code != 0)
    {
        return std::system_category().message(static_cast<int>(code));
    }
#endif
    return std::generic_category().message(errno);
}

} // namespace zipper

#endif // ZIPPER_UTILS_FILEPATH_HPP
