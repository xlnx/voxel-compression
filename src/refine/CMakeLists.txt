set(CMAKE_CXX_STANDARD 17)

set(VMCORE_SHARED_LIBRARY OFF)
vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/VMCore
  GIT_TAG        master
)

add_library(voxel_refine refiner.cc)
if (UNIX)
  target_link_libraries(voxel_refine pthread dl)
endif()
target_include_directories(voxel_refine PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
vm_target_dependency(voxel_refine VMUtils PUBLIC)
vm_target_dependency(voxel_refine vmcore PUBLIC)
target_link_libraries(voxel_refine vmcore)
