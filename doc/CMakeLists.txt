# https://cmake.org/cmake/help/latest/module/FindDoxygen.html
find_package(Doxygen)

if(NOT Doxygen_FOUND)
    return()
endif()

option(DOC_FOR_USER "Doxygen configured for user-facing documentation." TRUE)

# Extract static (and private) symbols when not user-facing:
if(DOC_FOR_USER)
    set(DOXYGEN_EXTRACT_STATIC NO)
else()
    set(DOXYGEN_EXTRACT_STATIC YES)
endif()
set(DOXYGEN_EXTRACT_PRIVATE DOXYGEN_EXTRACT_STATIC)

# Better output for C code:
set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)

# Do not ignore undocumented:
set(DOXYGEN_EXTRACT_ALL YES)

# Full source browser:
set(DOXYGEN_SOURCE_BROWSER YES)

# Inline the sources of functions:
set(DOXYGEN_INLINE_SOURCES YES)

# Do not include full path names in file name:
set(DOXYGEN_FULL_PATH_NAMES NO)

# Call/caller graphs:
set(DOXYGEN_CALL_GRAPH YES)
set(DOXYGEN_CALLER_GRAPH YES)

# HTML light/dark theme toggle:
set(DOXYGEN_HTML_COLORSTYLE "TOGGLE")

# Root README.md as main page:
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")

# Examples:
set(DOXYGEN_EXAMPLE_PATH "${PROJECT_SOURCE_DIR}/snippet")
set(DOXYGEN_EXAMPLE_RECURSIVE YES)

doxygen_add_docs(doxygen
    ${PROJECT_SOURCE_DIR}/README.md # needed for DOXYGEN_USE_MDFILE_AS_MAINPAGE
    ${PROJECT_SOURCE_DIR}/src
)
