##############################################################################
# Zipper CMake Configuration
# Main configuration file for building the Zipper library with CMake
##############################################################################

cmake_minimum_required(VERSION 3.12)
project(zipper)

##############################################################################
# Global compiler settings
##############################################################################
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

##############################################################################
# Build options for enabling/disabling demos and tests
##############################################################################
option(ZIPPER_BUILD_DEMOS "Build the demo applications" OFF)
option(ZIPPER_BUILD_TESTS "Build the test applications" OFF)

##############################################################################
# External dependencies
##############################################################################
# Configure zlib-ng
set(ZLIB_COMPAT ON CACHE BOOL "Enable zlib compatibility")
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "Disable zlib tests")
add_subdirectory(external/zlib-ng)

# Configure minizip with AES encryption support
# Define the CMP0077 policy to avoid warning with option() in external/minizip
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()
option(USE_AES "Enable AES encryption" ON)
add_subdirectory(external/minizip)
target_include_directories(minizip PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/external/zlib-ng>
)

##############################################################################
# Main library targets (static and shared)
##############################################################################
# Common source files definition
set(ZIPPER_SOURCES
    src/utils/Path.cpp
    src/utils/Timestamp.cpp
    src/Zipper.cpp
    src/Unzipper.cpp
)

##############################################################################
# Utility function to apply common configuration
##############################################################################
function(configure_zipper_target target)
    # Add platform-specific dependencies
    if(WIN32)
        target_sources(${target} PRIVATE
            src/utils/dirent.c
        )

        target_compile_definitions(${target} PRIVATE
            USE_WINDOWS=ON
            # Prevent Windows' min/max macros from conflicting with std::min/max
            NOMINMAX=ON
        )
    endif()

    # Include directories configuration
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        src
        external/minizip
    )

    # Public include directories
    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # Library dependencies
    target_link_libraries(${target} PRIVATE
        minizip
        zlibstatic
    )
endfunction()

# Static library
add_library(zipper STATIC ${ZIPPER_SOURCES})
configure_zipper_target(zipper)

# Shared library
add_library(zipper_shared SHARED ${ZIPPER_SOURCES})
set_target_properties(zipper_shared PROPERTIES
    OUTPUT_NAME zipper
    VERSION 2.0.0
    SOVERSION 2
)
configure_zipper_target(zipper_shared)

##############################################################################
# Demo applications
##############################################################################
# Build demos if option is enabled
if(ZIPPER_BUILD_DEMOS)
    message(STATUS "Building demo applications")

    # Configure the output directory for demos
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    # Add demos from their source location
    add_subdirectory(doc/demos/Zipper ${CMAKE_BINARY_DIR}/demos/Zipper)
    add_subdirectory(doc/demos/Unzipper ${CMAKE_BINARY_DIR}/demos/Unzipper)
endif()

##############################################################################
# Tests with Google Test
##############################################################################
# Build tests if option is enabled
if(ZIPPER_BUILD_TESTS)
    message(STATUS "Building test applications")
    add_subdirectory(tests)
    enable_testing()
endif()
