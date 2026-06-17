#[[
    Essential setup intended to be included at root CMakeLists.txt.
]]#

include_guard(GLOBAL)

message(FATAL_ERROR
    "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}, PROJECT_BINARY_DIR = ${PROJECT_BINARY_DIR}")

# Set the version.h file:
set(version_h_include_dir "${PROJECT_BINARY_DIR}/generated/c_objectpool")
# NOTE: PROJECT_BINARY_DIR is used instead to not dirty top-level project build directory.
set(version_h "${version_h_include_dir}/version.h")

find_package(Git) # https://cmake.org/cmake/help/latest/module/FindGit.html
if(Git_FOUND)
    # Set the commit hash as the run name (preferred).
    execute_process( # https://git-scm.com/docs/git-describe
        OUTPUT_VARIABLE git_commit_hash
        COMMAND git describe --always --dirty --long
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(git_commit_hash "???")
    # Set the ISO 8601 as the run name (redundant but good enough for uniqueness).
    string( # https://cmake.org/cmake/help/latest/command/string.html#timestamp
        TIMESTAMP codechecker_run_name
        "%Y-%m-%dT%H:%M:%SZ"
        UTC
    )
endif()
