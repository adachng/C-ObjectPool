#[[
    Unit tests.
#]]

project(
    c_objectpool_tests
    LANGUAGES C CXX
)

if(PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "This subdirectory exists only to add CXX as language requirement, and is not meant top level.")
endif()

add_subdirectory(googletest)

link_libraries(
    c_objectpool
    GTest::gtest_main # prefer this over GTest::gtest to have default main() provided
)

add_executable(test
    main_test.cpp
)
