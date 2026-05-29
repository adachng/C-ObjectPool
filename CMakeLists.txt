cmake_minimum_required(VERSION 3.22)

project(
    c_objectpool
    LANGUAGES C
)

list(PREPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(ProjectEssentials)

option(BUILD_SHARED_LIBS "Build using shared libraries." OFF)

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
