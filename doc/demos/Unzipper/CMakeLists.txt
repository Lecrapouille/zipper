##############################################################################
# Unzipper Demo CMake Configuration
##############################################################################

set(UNZIPPER_DEMO_SOURCES
    src/demo_unzip.cpp
)

# Define the demo executable
add_executable(unzipper-demo ${UNZIPPER_DEMO_SOURCES})

# Configure the include directories
target_include_directories(unzipper-demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Define important macros
target_compile_definitions(unzipper-demo PRIVATE
    HAVE_AES
)

# Link with the main library
target_link_libraries(unzipper-demo PRIVATE
    zipper
) 