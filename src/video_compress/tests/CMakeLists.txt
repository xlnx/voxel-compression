find_package(GTest REQUIRED)

file(GLOB_RECURSE VOCOMP_TEST_SRC
    test_*.cc
)

add_executable(vocomp_test_video_compress ${VOCOMP_TEST_SRC})
target_link_libraries(vocomp_test_video_compress gtest gtest_main pthread dl)
vm_target_dependency(vocomp_test_video_compress video_compress PRIVATE)

# if(VOCOMP_ENABLE_CUDA)
#     cuda_add_executable(vocomp_test_all ${VOCOMP_TEST_SRC})
# else()
#     add_executable(vocomp_test_all ${VOCOMP_TEST_SRC})
# endif()
# target_link_libraries(vocomp_test_all gtest gtest_main pthread dl)

# if(VOCOMP_BUILD_VIDEO_COMPRESS)
#     vm_target_dependency(vocomp_test_all video_compress PRIVATE)
# endif()
# if(VOCOMP_BUILD_VIDEO_DECOMPRESS)
#     vm_target_dependency(vocomp_test_all video_decompress PRIVATE)
# endif()
# if(VOCOMP_BUILD_REFINER)
#     vm_target_dependency(vocomp_test_all voxel_refine PRIVATE)
# endif()
# if(VOCOMP_BUILD_EXTRACTOR)
#     vm_target_dependency(vocomp_test_all voxel_extract PRIVATE)
# endif()
