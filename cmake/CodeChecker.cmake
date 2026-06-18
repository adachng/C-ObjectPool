#[[
    Convenience for CodeChecker.
]]#

include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

set(CODECHECKER_BIN
    "CodeChecker"
    CACHE
    STRING
    "CodeChecker command."
)

find_program(CODECHECKER_EXE
    NAMES ${CODECHECKER_BIN}
    DOC "CodeChecker executable."
)

if(NOT CODECHECKER_EXE)
    return()
endif()

set(CODECHECKER_URL
    "127.0.0.1:8001/${CMAKE_PROJECT_NAME}"
    CACHE
    STRING
    "CodeChecker URL for store command."
)

set(CODECHECKER_REPORTS_DIR
    "${PROJECT_BINARY_DIR}/.codechecker_reports"
    CACHE
    STRING
    "CodeChecker URL for store command."
)

find_package(Git) # https://cmake.org/cmake/help/latest/module/FindGit.html
if(Git_FOUND)
    # Set the commit hash as the run name (preferred).
    execute_process( # https://git-scm.com/docs/git-describe
        OUTPUT_VARIABLE codechecker_run_name
        COMMAND git describe --always --dirty --long
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    # Set the ISO 8601 as the run name (redundant but good enough for uniqueness).
    string( # https://cmake.org/cmake/help/latest/command/string.html#timestamp
        TIMESTAMP codechecker_run_name
        "%Y-%m-%dT%H:%M:%SZ"
        UTC
    )
endif()

add_custom_target(codechecker
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    VERBATIM
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CODECHECKER_REPORTS_DIR}
    COMMAND ${CODECHECKER_BIN} analyze
        ${CMAKE_BINARY_DIR}/compile_commands.json
        -o ${CODECHECKER_REPORTS_DIR}
        -i ${PROJECT_SOURCE_DIR}/.skipfile
        --enable-all
        --analyzers clangsa clang-tidy gcc
        --analyzer-config clang-tidy:take-config-from-directory=true
        -n ${codechecker_run_name}
    COMMAND ${CODECHECKER_BIN} store
            ${CODECHECKER_REPORTS_DIR}
            --trim-path-prefix ${PROJECT_SOURCE_DIR}
            --url ${CODECHECKER_URL}
)
