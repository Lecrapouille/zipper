target_sources(zipper PRIVATE
    Path.cpp
    Timestamp.cpp
    glob.cpp
)

# Add platform-specific dependencies
if(WIN32)
    target_sources(zipper PRIVATE
        dirent.c
    )
    target_compile_definitions(zipper PRIVATE
        # Prevent Windows' min/max macros from conflicting with std::min/max
        NOMINMAX=ON
    )
endif()
