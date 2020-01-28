set(CMAKE_CXX_STANDARD 14)

vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cuda-fx
  GIT_TAG        master
)

file(GLOB_RECURSE VARCH_UNARCHIVE_SRC
  *.cc
  **/*.cc
  **/*.cu
)

cuda_add_library(voxel_unarchive ${VARCH_UNARCHIVE_SRC})

vm_target_dependency(voxel_unarchive vmcore PRIVATE)
vm_target_dependency(voxel_unarchive VMUtils PUBLIC)
vm_target_dependency(voxel_unarchive cudafx PUBLIC)
target_link_libraries(voxel_unarchive ${CUDA_CUDA_LIBRARY})

if(MSVC)
  if(CMAKE_CL_64)
    target_link_libraries(voxel_unarchive
      ${PROJECT_SOURCE_DIR}/lib/x64/nvcuvid.lib
    )
  else()
    target_link_libraries(voxel_unarchive
      ${PROJECT_SOURCE_DIR}/lib/Win32/nvcuvid.lib
    )
  endif()
else()
  target_link_libraries(voxel_unarchive
    ${PROJECT_SOURCE_DIR}/lib/linux/stubs/x86_64/libnvcuvid.so
  )
endif()

target_include_directories(voxel_unarchive PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
