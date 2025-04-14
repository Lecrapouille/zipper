//-----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//-----------------------------------------------------------------------------

#ifndef ZIPPER_UTILS_OS_HPP
#  define ZIPPER_UTILS_OS_HPP

extern "C"
{
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <time.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/stat.h>

#  if defined(USE_WINDOWS)
#    define DIRECTORY_SEPARATOR "\\"
#    define DIRECTORY_SEPARATOR_CHAR '\\'
#  else
#    define DIRECTORY_SEPARATOR "/"
#    define DIRECTORY_SEPARATOR_CHAR '/'
#  endif

#  if defined(USE_WINDOWS)
#    include <direct.h>
#    include <io.h>
    typedef struct _stat STAT;
#    define stat _stat
#    define S_IFREG _S_IFREG
#    define S_IFDIR _S_IFDIR
#    define access _access
#    define mkdir _mkdir
#    define rmdir _rmdir
#  else
#    include <sys/types.h>
#    include <dirent.h>
#    include <unistd.h>
#    include <utime.h>
typedef struct stat STAT;
#  endif

#  if defined(USE_WINDOWS)
#    define USEWIN32IOAPI
#    include "external/minizip/ioapi.h"
#    include "external/minizip/iowin32.h"
#  endif

} // extern C

#  if defined(_WIN64) && (!defined(__APPLE__))
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
#  endif

#  if defined(USE_WINDOWS)
#    define MKDIR(d) _mkdir(d)
#    define CHDIR(d) _chdir(d)
#  else
#    define MKDIR(d) mkdir(d, 0775)
#    define CHDIR(d) chdir(d)
#  endif

#endif // ZIPPER_UTILS_OS_HPP
