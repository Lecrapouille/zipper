#!/bin/bash -e
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

source ../.makefile/compile-external-libs.sh

### Library zlib-ng
print-compile zlib-ng
if [ -e zlib-ng ];
then
    mkdir -p zlib-ng/build
    (cd zlib-ng/build
     call-cmake -DZLIB_COMPAT=ON -DZLIB_ENABLE_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
     call-make
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
     call-cmake -DUSE_AES=ON ..
     call-make
    )
else
    echo "Failed compiling external/zipper: directory does not exist"
fi
