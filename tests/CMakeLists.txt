find_package(GTest REQUIRED)

set(VOCOMP_TEST_SRC)

if(VOCOMP_BUILD_VIDEO_COMPRESS AND VOCOMP_BUILD_VIDEO_DECOMPRESS)
    file(GLOB_RECURSE VOCOMP_TEST_VIDEO_DECOMPRESS_SRC
        test_video_decomp*.cc
    )
    set(VOCOMP_TEST_SRC ${VOCOMP_TEST_SRC} ${VOCOMP_TEST_VIDEO_DECOMPRESS_SRC})
endif()

if(VOCOMP_ENABLE_CUDA)
    cuda_add_executable(vocomp_test_all ${VOCOMP_TEST_SRC})
else()
    add_executable(vocomp_test_all ${VOCOMP_TEST_SRC})
endif()
target_link_libraries(vocomp_test_all gtest gtest_main pthread dl)

if(VOCOMP_BUILD_VIDEO_COMPRESS)
    vm_target_dependency(vocomp_test_all video_compress PRIVATE)
endif()
if(VOCOMP_BUILD_VIDEO_DECOMPRESS)
    vm_target_dependency(vocomp_test_all video_decompress PRIVATE)
endif()
if(VOCOMP_BUILD_REFINER)
    vm_target_dependency(vocomp_test_all voxel_refine PRIVATE)
endif()
if(VOCOMP_BUILD_EXTRACTOR)
    vm_target_dependency(vocomp_test_all voxel_extract PRIVATE)
endif()
