target_sources(zipper PRIVATE
    Zipper.cpp
    Unzipper.cpp
)
add_subdirectory(utils)

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

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zipper_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
else()
    # For all other cases, undefine the macro
    target_compile_options(zipper PRIVATE -UZIPPER_EXPORT_DEFINED)
endif()

# Include directories configuration
target_include_directories(zipper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Public include directories
target_include_directories(zipper PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    # for the zipper_export.h file. Is it needed though?
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Library dependencies
target_link_libraries(zipper PRIVATE
    MINIZIP::minizip
    # TODO: Use the non-compat version
    ZLIB::zlib
)

install(TARGETS zipper
    EXPORT zipperTargets
)
