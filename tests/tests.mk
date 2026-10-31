###################################################
# Unit tests, with Google Test. Paths are relative to this directory.
###################################################

TARGETS += zipper-tests
zipper-tests.TYPE := test
zipper-tests.DESCRIPTION := Unit tests for zipper
zipper-tests.SOURCES := **/*.cpp
zipper-tests.INCLUDES := .. ../src
zipper-tests.DEFINES := HAVE_AES PWD="$(abspath $(HERE))"
zipper-tests.CXXFLAGS := -Wno-null-dereference
zipper-tests.DEPENDS := zipper
# The tests write and remove files (doc/, data/...) in their directory.
zipper-tests.WORKDIR := .
zipper-tests.PKG_CONFIG := gtest gmock
