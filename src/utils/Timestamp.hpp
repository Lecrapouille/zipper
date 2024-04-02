//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#ifndef TIMESTAMP_HPP
#  define TIMESTAMP_HPP

#  if defined(USE_WINDOWS)
#    include <Windows.h>
#  elif __linux__
#    include <sys/stat.h>
#  endif

#  include <string>
#  include <time.h>

namespace zipper {

// *****************************************************************************
//! \brief Creates a timestap either from file or just current time If it fails
//! to read the timestamp of a file, it set the time stamp to current time
//!
//! \warning It uses std::time to get current time, which is not standardized to
//! be 1970-01-01....  However, it works on Windows and Unix
//! https://stackoverflow.com/questions/6012663/get-unix-timestamp-with-c With
//! C++20 this will be standardized
// *****************************************************************************
struct Timestamp
{
    Timestamp();
    Timestamp(const std::string& filepath);

    tm timestamp;
};

} // namespace

#endif // TIMESTAMP_HPP
