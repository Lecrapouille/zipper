#!/bin/bash -e
###############################################################################
### This script is called by (cd .. && make download-external-libs). It will
### git clone thirdparts needed for this project but does not compile them.
### It replaces git submodules that I dislike.
###############################################################################

source ../.makefile/download-external-libs.sh

### zlib replacement with optimizations for "next generation" systems.
### License: zlib
cloning zlib-ng/zlib-ng

### Minizip contrib in zlib with latest bug fixes that supports PKWARE disk
### spanning, AES encryption, and IO buffering.
### License: zlib
cloning Lecrapouille/minizip
