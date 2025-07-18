##############################################################################
# Zipper CMake Configuration
# Main configuration file for building the Zipper library with CMake
##############################################################################

cmake_minimum_required(VERSION 3.12)
project(zipper VERSION 2.2.0)

##############################################################################
# Global compiler settings
##############################################################################
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

##############################################################################
# Build options
##############################################################################
option(ZIPPER_BUILD_DEMOS "Build the demo applications" OFF)
option(ZIPPER_BUILD_TESTS "Build the test applications" OFF)
option(ZIPPER_SHARED_LIB "Build zipper as a shared library" OFF)
option(USE_LOCAL_SUBMODULES "Use local submodules instead of FetchContent" OFF)

# Force static libraries for dependencies on Windows to avoid DLL issues
if(WIN32)
    set(BUILD_SHARED_LIBS OFF)
endif()

##############################################################################
# Include FetchContent if not using local submodules or if they don't exist
##############################################################################

set(NEED_FETCH_CONTENT_ZLIB_NG FALSE)
set(NEED_FETCH_CONTENT_MINIZIP FALSE)

if(NOT (USE_LOCAL_SUBMODULES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib-ng/CMakeLists.txt"))
    set(NEED_FETCH_CONTENT_ZLIB_NG TRUE)
    message(STATUS "Using FetchContent for zlib-ng")
else()
    message(STATUS "Using local zlib-ng submodule")
endif()

if(NOT (USE_LOCAL_SUBMODULES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/minizip/CMakeLists.txt"))
    set(NEED_FETCH_CONTENT_MINIZIP TRUE)
    message(STATUS "Using FetchContent for minizip")
else()
    message(STATUS "Using local minizip submodule")
endif()

if(NEED_FETCH_CONTENT_ZLIB_NG OR NEED_FETCH_CONTENT_MINIZIP)
    include(FetchContent)
endif()

##############################################################################
# Configure zlib-ng third party dependency
##############################################################################
set(ZLIB_COMPAT ON CACHE BOOL "Enable zlib-ng compatibility")
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "Disable zlib-ng tests")
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip installation of zlib-ng")

# Use zlib-ng from local submodule if available, otherwise use FetchContent
if(NOT NEED_FETCH_CONTENT_ZLIB_NG)
    add_subdirectory(external/zlib-ng)
else()
    FetchContent_Declare(zlib-ng
        GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
        GIT_TAG        2.2.4
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(zlib-ng)
endif()

##############################################################################
# Configure minizip third party dependency
##############################################################################

# Enable AES encryption instead of default option
option(USE_AES "Enable AES encryption" ON)

# Define the CMP0077 policy to avoid warning with option() in external/minizip
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

# Use minizip from local submodule if available, otherwise use FetchContent
if(NOT NEED_FETCH_CONTENT_MINIZIP)
    add_subdirectory(external/minizip)
    target_include_directories(minizip PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/external/zlib-ng>
    )
else()
    FetchContent_Declare(minizip
        GIT_REPOSITORY https://github.com/Lecrapouille/minizip.git
        GIT_TAG        v1.2
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(minizip)

    # Ensure minizip has access to zlib-ng headers
    if(TARGET minizip)
        target_include_directories(minizip PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/_deps/zlib-ng-build>
        )
    endif()
endif()

##############################################################################
# Main library targets (static and shared)
##############################################################################
# Common source files definition
set(ZIPPER_SOURCES
    src/utils/Path.cpp
    src/utils/Timestamp.cpp
    src/utils/glob.cpp
    src/Zipper.cpp
    src/Unzipper.cpp
)

# Compile as static or as shared library
if(ZIPPER_SHARED_LIB)
    add_library(zipper SHARED ${ZIPPER_SOURCES})
else()
    add_library(zipper STATIC ${ZIPPER_SOURCES})
endif()

# Add platform-specific dependencies
if(WIN32)
    target_sources(zipper PRIVATE
        src/utils/dirent.c
    )

    target_compile_definitions(zipper PRIVATE
        # Prevent Windows' min/max macros from conflicting with std::min/max
        NOMINMAX=ON
    )
endif()

# Export symbols for Windows and DLLs.
if(WIN32 AND ZIPPER_SHARED_LIB)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

    # Include and setup for exporting symbols
    include(GenerateExportHeader)

    # Generate export header for shared library symbols
    generate_export_header(zipper
        BASE_NAME zipper
        EXPORT_MACRO_NAME ZIPPER_EXPORT
        EXPORT_FILE_NAME zipper_export.h
    )
    target_compile_definitions(zipper PRIVATE zipper_EXPORTS)
    target_compile_definitions(zipper PUBLIC ZIPPER_EXPORT_DEFINED)
else()
    # For all other cases, undefine the macro
    target_compile_options(zipper PRIVATE -UZIPPER_EXPORT_DEFINED)
endif()

# Include directories configuration
target_include_directories(zipper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    src
    external/minizip
)

# Public include directories
target_include_directories(zipper PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Library dependencies
target_link_libraries(zipper PRIVATE
    minizip
    zlibstatic
)

##############################################################################
# Build demos if CMake option is enabled
##############################################################################
if(ZIPPER_BUILD_DEMOS)
    message(STATUS "Building demo applications")

    # Configure the output directory for demos
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    # Add demos from their source location
    add_subdirectory(doc/demos/Zipper ${CMAKE_BINARY_DIR}/demos/Zipper)
    add_subdirectory(doc/demos/Unzipper ${CMAKE_BINARY_DIR}/demos/Unzipper)
endif()

##############################################################################
# Build tests if CMake option is enabled (use Google Test)
##############################################################################
if(ZIPPER_BUILD_TESTS)
    message(STATUS "Building test applications")
    enable_testing()
    add_subdirectory(tests)
endif()

##############################################################################
# Configure and install pkg-config file
##############################################################################
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/zipper.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/zipper.pc
    @ONLY
)

##############################################################################
# Installation rules
##############################################################################
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Configure version information for shared libraries
if(ZIPPER_SHARED_LIB)
    set_target_properties(zipper PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

install(TARGETS zipper
    EXPORT zipperTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        NAMELINK_SKIP
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# For shared libraries, add NAMELINK installation
if(ZIPPER_SHARED_LIB)
    install(TARGETS zipper
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            NAMELINK_ONLY
    )
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(WIN32 AND ZIPPER_SHARED_LIB)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zipper_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zipper.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

##############################################################################
# CMake Package Configuration (for find_package support)
##############################################################################

# Install the export targets
install(EXPORT zipperTargets
    FILE zipperTargets.cmake
    NAMESPACE zipper::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zipper
)

# Generate the config file
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/zipperConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zipper
    PATH_VARS INCLUDE_INSTALL_DIR
)

# Generate the version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install the configuration files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zipper
)
