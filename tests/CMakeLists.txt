##############################################################################
# Tests CMake Configuration
##############################################################################

# Needed for GoogleTest
set(CMAKE_CXX_STANDARD 14)

# Force Google Test to use shared CRT on Windows (/MD or /MDd)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Download and configure GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Define the test sources
set(TESTS_SOURCES
    main.cpp
    TestZip.cpp
    TestPath.cpp
)

# Define the test executable
add_executable(zipper-tests ${TESTS_SOURCES})

# Configure the include directories
target_include_directories(zipper-tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Define important macros
target_compile_definitions(zipper-tests PRIVATE
    PWD="${CMAKE_CURRENT_SOURCE_DIR}"
    HAVE_AES
)

# Link with the main library and Google Test
target_link_libraries(zipper-tests PRIVATE
    zipper
    gtest
    gtest_main
    gmock
)

# Add the test to the CTest list
add_test(NAME zipper-tests COMMAND zipper-tests)