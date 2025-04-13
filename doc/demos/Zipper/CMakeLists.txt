##############################################################################
# Zipper Demo CMake Configuration
##############################################################################

set(ZIPPER_DEMO_SOURCES
    src/demo_zip.cpp
)

set(CMAKE_CXX_STANDARD 17)

# Define the demo executable
add_executable(zipper-demo ${ZIPPER_DEMO_SOURCES})

# Configure the include directories
target_include_directories(zipper-demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Define important macros
target_compile_definitions(zipper-demo PRIVATE
    HAVE_AES
)

# Link with the main library
target_link_libraries(zipper-demo PRIVATE
    zipper
)