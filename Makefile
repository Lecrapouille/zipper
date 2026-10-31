###################################################
# Zipper: a C++ library for creating and reading zip archives, built by
# MyMakefile 3. CMake users: see CMakeLists.txt.
###################################################

PROJECT_NAME := zipper
PROJECT_VERSION := 4.0.0
LICENSE := MIT
CXX_STANDARD := c++20
COMPILATION_MODE ?= release
# Headers in include/zipper/<version>/, as MyMakefile 2 installed them.
INSTALL_VERSIONED ?= 1
PROJECT_LOGO := doc/doxygen-logo.png

###################################################
# The library, holding minizip and zlib-ng.
#
TARGETS := zipper
zipper.TYPE := library
zipper.DESCRIPTION := Zipper is a C++ library for creating and reading zip archives.
zipper.SOURCES = src/**/*.cpp $(if $(filter Windows,$(TARGET_OS)),src/utils/dirent.c)
zipper.PUBLIC_INCLUDES := include
zipper.INCLUDES := src .
zipper.DEFINES := HAVE_AES
zipper.DEPENDS := minizip zlib-ng
zipper.MERGE := minizip zlib-ng

###################################################
# The other targets, each declared in the directory of its sources.
#
SUBMAKEFILES := external/external.mk tests/tests.mk \
    doc/demos/Unzipper/Unzipper.mk doc/demos/Zipper/Zipper.mk

M ?= .makefile
include $(M)/MyMakefile.mk
