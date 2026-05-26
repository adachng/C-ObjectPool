#[[
    Essential setup intended to be included at root CMakeLists.txt.
]]#

include_guard(GLOBAL)

if(PROJECT_IS_TOP_LEVEL)
    # Set the target output locations:
    set(stage_dir ${PROJECT_BINARY_DIR}/stage)
    include(GNUInstallDirs)
    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${stage_dir}/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
    endif()
    if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${stage_dir}/$<CONFIG>/${CMAKE_INSTALL_LIBDIR})
    endif()
    if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${stage_dir}/$<CONFIG>/${CMAKE_INSTALL_LIBDIR})
    endif()
    unset(stage_dir)
endif()
