cmake_minimum_required(VERSION 3.12)
project(zipper)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(
    ZLIB_COMPAT ON
    ZLIB_ENABLE_TESTS OFF
)
add_subdirectory(external/zlib-ng)

set(USE_AES ON)
add_subdirectory(external/minizip)
target_include_directories(minizip PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/external/zlib-ng>
)
add_library(zipper STATIC 
    src/utils/Path.cpp
    src/utils/Timestamp.cpp
    src/Zipper.cpp
    src/Unzipper.cpp
)

if(WIN32)
target_sources(zipper PRIVATE
    src/utils/dirent.c
)
target_compile_definitions(zipper PRIVATE
    USE_WINDOWS=ON
    NOMINMAX=ON
)
endif()

target_include_directories(zipper PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    src
    external/minizip
)

target_include_directories(zipper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(zipper PRIVATE
    minizip
    zlibstatic
)
