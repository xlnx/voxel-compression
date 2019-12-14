find_package(GTest REQUIRED)

file(GLOB_RECURSE VOCOMP_TEST_SRC
    test_*.cc
)

add_executable(vocomp_test_video_compress ${VOCOMP_TEST_SRC})
target_link_libraries(vocomp_test_video_compress gtest gtest_main pthread dl)
vm_target_dependency(vocomp_test_video_compress video_compress PRIVATE)
