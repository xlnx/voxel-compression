find_package(GTest REQUIRED)

file(GLOB_RECURSE VOCOMP_TEST_SRC
    test_*.cc
)

add_executable(vocomp_test_utils ${VOCOMP_TEST_SRC})
target_link_libraries(vocomp_test_utils gtest gtest_main pthread dl)
