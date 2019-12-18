find_package(GTest REQUIRED)

file(GLOB_RECURSE VOCOMP_TEST_SRC
    test_*.cc
)

add_executable(vocomp_test_video_decompress ${VOCOMP_TEST_SRC})
target_link_libraries(vocomp_test_video_decompress gtest gtest_main pthread dl)
vm_target_dependency(vocomp_test_video_decompress video_compress PRIVATE)
vm_target_dependency(vocomp_test_video_decompress video_decompress PRIVATE)
