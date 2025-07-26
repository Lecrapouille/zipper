##############################################################################
# Zipper CMake Configuration
# Main configuration file for building the Zipper library with CMake
##############################################################################

cmake_minimum_required(VERSION 3.12)
project(zipper VERSION 3.0.2)

##############################################################################
# Global compiler settings
##############################################################################
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(FetchContent)
include(CMakePackageConfigHelpers)

##############################################################################
# Build options
##############################################################################
option(ZIPPER_BUILD_DEMOS "Build the demo applications" OFF)
option(ZIPPER_BUILD_TESTS "Build the test applications" OFF)
option(ZIPPER_SHARED_LIB "Build zipper as a shared library" OFF)
option(ZIPPER_USE_LOCAL_SUBMODULES "Use local submodules instead of FetchContent" OFF)

##############################################################################
# Third-party dependencies
##############################################################################

add_subdirectory(external)

##############################################################################
# Main library targets
##############################################################################

# Compile as static or as shared library
if(ZIPPER_SHARED_LIB)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

add_library(zipper)
add_library(zipper::zipper ALIAS zipper)

set_target_properties(zipper PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Main implementation in here
add_subdirectory(src)
add_subdirectory(include)

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
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zipper.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

##############################################################################
# CMake Package Configuration (for find_package support)
##############################################################################
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Export the installed targets
install(EXPORT zipperTargets
    FILE zipperTargets.cmake
    NAMESPACE zipper::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zipper
)
# Export for build-tree targets also
export(EXPORT zipperTargets
    FILE zipperTargets.cmake
    NAMESPACE zipper::
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/zipperConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zipper
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
)

# Install the configuration files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/zipperConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zipper
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
