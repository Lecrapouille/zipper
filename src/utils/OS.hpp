//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#ifndef ZIPPER_UTILS_OS_HPP
#define ZIPPER_UTILS_OS_HPP

extern "C"
{
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define WINDOWS_DIRECTORY_SEPARATOR '\\'
#define UNIX_DIRECTORY_SEPARATOR '/'
#if defined(_WIN32)
#    define DIRECTORY_SEPARATOR WINDOWS_DIRECTORY_SEPARATOR
#else
#    define DIRECTORY_SEPARATOR UNIX_DIRECTORY_SEPARATOR
#endif

#if defined(_WIN32)
#    include <direct.h>
#    include <io.h>
    typedef struct _stat STAT;
#    define stat _stat
#    define S_IFREG _S_IFREG
#    define S_IFDIR _S_IFDIR
#    define access _access
#    define mkdir _mkdir
#    define rmdir _rmdir
#else
#    include <dirent.h>
#    include <sys/types.h>
#    include <unistd.h>
#    include <utime.h>
    typedef struct stat STAT;
#endif

#if defined(_WIN32)
#    define USEWIN32IOAPI
#    include "external/minizip/ioapi.h"
#    include "external/minizip/iowin32.h"
#endif

} // extern C

#if defined(_WIN64) && (!defined(__APPLE__))
#    ifndef __USE_FILE_OFFSET64
#        define __USE_FILE_OFFSET64
#    endif
#    ifndef __USE_LARGEFILE64
#        define __USE_LARGEFILE64
#    endif
#    ifndef _LARGEFILE64_SOURCE
#        define _LARGEFILE64_SOURCE
#    endif
#    ifndef _FILE_OFFSET_BIT
#        define _FILE_OFFSET_BIT 64
#    endif
#endif

#if defined(_WIN32)
#    define OS_STRERROR ::strerror // FIXME win_strerror
#    define OS_MKDIR(d, v) ::_mkdir(d)
#    define OS_CHDIR(d) ::_chdir(d)
#    define OS_GETCWD(b, s) ::_getcwd(b, s)
#    define OS_UNLINK(f) ::_unlink(f)
#    define OS_RMDIR(d) ::_rmdir(d)
#    define OS_REMOVE(f) ::remove(f)
#    define OS_RENAME(o, n) ::rename(o, n)
#else
#    define OS_STRERROR ::strerror
#    define OS_MKDIR(d, v) ::mkdir(d, v)
#    define OS_CHDIR(d) ::chdir(d)
#    define OS_GETCWD(b, s) ::getcwd(b, s)
#    define OS_UNLINK(f) ::unlink(f)
#    define OS_RMDIR(d) ::rmdir(d)
#    define OS_REMOVE(f) ::remove(f)
#    define OS_RENAME(o, n) ::rename(o, n)
#endif

#endif // ZIPPER_UTILS_OS_HPP
