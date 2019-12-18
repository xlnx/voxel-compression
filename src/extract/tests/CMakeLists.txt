find_package(GTest REQUIRED)

file(GLOB_RECURSE VOCOMP_TEST_SRC
    test_*.cc
)

cuda_add_executable(vocomp_test_extractor ${VOCOMP_TEST_SRC})
target_link_libraries(vocomp_test_extractor gtest gtest_main pthread dl)
vm_target_dependency(vocomp_test_extractor voxel_extract PRIVATE)
vm_target_dependency(vocomp_test_extractor voxel_refine PRIVATE)
target_include_directories(vocomp_test_extractor PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
