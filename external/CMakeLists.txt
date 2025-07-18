# Support for FetchContent/find_package integration using CMake 3.24
set(fetch_content_extra_args)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    list(APPEND fetch_content_extra_args
        FIND_PACKAGE_ARGS CONFIG
    )
endif()

# Declare the dependencies
# Some default options are overwritten to better support bundled buildings
option(WITH_GTEST         "zipper: Override" OFF)
option(WITH_FUZZERS       "zipper: Override" OFF)
option(WITH_BENCHMARKS    "zipper: Override" OFF)
option(ZLIB_COMPAT        "zipper: Override" ON)
option(ZLIB_ENABLE_TESTS  "zipper: Override" OFF)
option(SKIP_INSTALL_ALL   "zipper: Override" ON)
FetchContent_Declare(zlib-ng
    GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
    GIT_TAG        2.2.4
    ${fetch_content_extra_args}
)

option(USE_AES            "zipper: Override" ON)
FetchContent_Declare(minizip
    GIT_REPOSITORY https://github.com/Lecrapouille/minizip.git
    GIT_TAG        v1.2
    ${fetch_content_extra_args}
)

# If we are asked to use submodules, override the paths to point to
# the submodule destination
if(USE_LOCAL_SUBMODULES)
    set(FETCHCONTENT_SOURCE_DIR_MINIZIP ${CMAKE_CURRENT_SOURCE_DIR}/minizip CACHE PATH
        "zipper: Override (USE_LOCAL_SUBMODULES)"
    )
    set(FETCHCONTENT_SOURCE_DIR_ZLIB-NG ${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng CACHE PATH
        "zipper: Override (USE_LOCAL_SUBMODULES)"
    )
endif()

# Make all dependencies build as static libraries
# TODO: This can be scoped for each dependency, but requires
#   reorganization of the folder structure or CMake 3.25
set(BUILD_SHARED_LIBS OFF)

# Do the actual add_subdirectory/find_package calls
FetchContent_MakeAvailable(minizip zlib-ng)

# Additional compatibility support because the dependencies do not properly support
# FetchContent (gated by <PROJECT_NAME>_SOURCE_DIR) or find_package is not compatible
if(minizip_SOURCE_DIR)
    # Need to add include directories
    target_include_directories(minizip PUBLIC
        $<BUILD_INTERFACE:${minizip_SOURCE_DIR}>
    )
endif()
