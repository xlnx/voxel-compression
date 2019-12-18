set(CMAKE_CXX_STANDARD 14)

vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cuda-fx
  GIT_TAG        master
)

cuda_add_library(voxel_extract extractor.cc)
target_include_directories(voxel_extract PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
vm_target_dependency(voxel_extract VMUtils PUBLIC)
vm_target_dependency(voxel_extract cudafx PUBLIC)
vm_target_dependency(voxel_extract video_decompress PUBLIC)

if(VOCOMP_BUILD_TESTS)
  add_subdirectory(tests)
endif()
