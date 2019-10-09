set(CMAKE_CXX_STANDARD 14)

add_library(voxel_extract extractor.cc)
target_include_directories(voxel_extract PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
vm_target_dependency(voxel_extract VMUtils PUBLIC)

