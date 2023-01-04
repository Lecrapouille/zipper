#!/bin/bash -ex

###############################################################################
### This script is called by (cd .. && make compile-external-libs). It will
### compile thirdparts cloned previously with make download-external-libs.
###
### To avoid pollution, these libraries are not installed in your operating
### system (no sudo make install is called). As consequence, you have to tell
### your project ../Makefile where to find their files. Here generic example:
###     INCLUDES += -I$(THIRDPART)/thirdpart1/path1
###        for searching heeder files.
###     VPATH += $(THIRDPART)/thirdpart1/path1
###        for searching c/c++ files.
###     THIRDPART_LIBS += $(abspath $(THIRDPART)/libXXX.a))
###        for linking your project against the lib.
###     THIRDPART_OBJS += foo.o
###        for inking your project against this file iff THIRDPART_LIBS is not
###        used (the path is not important thanks to VPATH).
###
### The last important point to avoid polution, better to compile thirdparts as
### static library rather than shared lib to avoid telling your system where to
### find them when you'll start your application.
###############################################################################

### $1 is given by ../Makefile and refers to the current architecture.
if [ "$1" == "" ]; then
  echo "Expected one argument. Select the architecture: Linux, Darwin or Windows"
  exit 1
fi

ARCHI="$1"
TARGET="$2"
CC="$3"
CXX="$4"

function print-compile
{
    echo -e "\033[35m*** Compiling:\033[00m \033[36m$TARGET\033[00m <= \033[33m$1\033[00m"
}

### Number of CPU cores
NPROC=-j1
if [[ "$ARCHI" == "Darwin" ]]; then
    NPROC=-j`sysctl -n hw.logicalcpu`
elif [[ "$ARCHI" == "Linux" ]]; then
    NPROC=-j`nproc --all`
fi

### Library zlib-ng
print-compile zlib-ng
if [ -e zlib-ng ];
then
    mkdir -p zlib-ng/build
    (cd zlib-ng/build
     cmake -GNinja -DZLIB_COMPAT=ON -DZLIB_ENABLE_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
     VERBOSE=1 ninja $NPROC
    )
else
    echo "Failed compiling external/zipper: directory does not exist"
fi

### Library minizip
print-compile minizip
if [ -e minizip ];
then
    mkdir -p minizip/build
    (cd minizip/build
     cmake -GNinja -DUSE_AES=ON ..
     VERBOSE=1 ninja $NPROC
    )
else
    echo "Failed compiling external/zipper: directory does not exist"
fi
