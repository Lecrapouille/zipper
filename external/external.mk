###################################################
# Third parties: the git submodules external/zlib-ng and external/minizip,
# built by their CMake. The sources of Zipper include
# "external/minizip/zip.h": they stay in external/.
###################################################

TARGETS += zlib-ng minizip

zlib-ng.PATH := zlib-ng
zlib-ng.TYPE := cmake
zlib-ng.CMAKE_ARGS := -DZLIB_COMPAT=ON -DZLIB_ENABLE_TESTS=OFF -DWITH_GTEST=OFF
zlib-ng.LIBRARIES := z

minizip.PATH := minizip
minizip.TYPE := cmake
minizip.CMAKE_ARGS := -DUSE_AES=ON
minizip.LIBRARIES := minizip aes
