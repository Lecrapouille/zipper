//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#include "Timestamp.hpp"
#include <chrono>
#include <ctime>
#if !defined(_WIN32)
#    include <sys/stat.h>
#endif

using namespace zipper;

// -----------------------------------------------------------------------------
// Helper function for safe localtime access across platforms
// -----------------------------------------------------------------------------
static inline tm* safe_localtime(const time_t* time, tm* result)
{
#if defined(_WIN32)
    // Use localtime_s on Windows which is thread-safe
    if (localtime_s(result, time) != 0)
    {
        return nullptr;
    }
    return result;
#else
// On Unix-like systems
#    if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 1 || \
        defined(_XOPEN_SOURCE) || defined(_BSD_SOURCE) ||   \
        defined(_SVID_SOURCE) || defined(_POSIX_SOURCE)
    // Use localtime_r if available (thread-safe)
    return localtime_r(time, result);
#    else
    // Fallback to localtime on platforms without localtime_r
    // Note: This is not thread-safe
    const tm* tmp = std::localtime(time);
    if (tmp == nullptr)
    {
        return nullptr;
    }
    *result = *tmp;
    return result;
#    endif
#endif
}

// -----------------------------------------------------------------------------
Timestamp::Timestamp()
{
    std::time_t now = std::time(nullptr);
    tm temp_tm;
    if (safe_localtime(&now, &temp_tm) != nullptr)
    {
        timestamp = temp_tm;
    }
}

// -----------------------------------------------------------------------------
Timestamp::Timestamp(const std::string& filepath)
{
    // Set default
    std::time_t now = std::time(nullptr);
    tm temp_tm;
    if (safe_localtime(&now, &temp_tm) != nullptr)
    {
        timestamp = temp_tm;
    }

#if defined(_WIN32)

    // Implementation based on Ian Boyd's
    // https://stackoverflow.com/questions/20370920/convert-current-time-from-windows-to-unix-timestamp-in-c-or-c
    HANDLE hFile1;
    FILETIME filetime;
    hFile1 = CreateFile(filepath.c_str(),
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        nullptr,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        nullptr);

    if (hFile1 == INVALID_HANDLE_VALUE)
    {
        return;
    }

    if (!GetFileTime(hFile1, &filetime, nullptr, nullptr))
    {
        CloseHandle(hFile1);
        return;
    }
    const int64_t UNIX_TIME_START =
        0x019DB1DED53E8000; // January 1, 1970 (start of Unix epoch) in "ticks"
    const int64_t TICKS_PER_SECOND = 10000000; // a tick is 100ns

    // Copy the low and high parts of FILETIME into a LARGE_INTEGER
    // This is so we can access the full 64-bits as an Int64 without causing an
    // alignment fault.
    LARGE_INTEGER li;
    li.LowPart = filetime.dwLowDateTime;
    li.HighPart = filetime.dwHighDateTime;

    // Convert ticks since 1/1/1970 into seconds
    time_t time_s = (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;

    safe_localtime(&time_s, &timestamp);
    CloseHandle(hFile1);

#else // !_WIN32

    struct stat buf;
    if (stat(filepath.c_str(), &buf) != 0)
    {
        return;
    }

#    if defined(__APPLE__)
    auto timet = static_cast<time_t>(buf.st_mtimespec.tv_sec);
#    else
    auto timet = buf.st_mtim.tv_sec;
#    endif

    safe_localtime(&timet, &timestamp);

#endif // _WIN32
}
