set(CMAKE_CXX_STANDARD 14)

vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cuda-fx
  GIT_TAG        master
)

file(GLOB_RECURSE VOCOMP_EXTRACT_SRC
  *.cc
  **/*.cc
  **/*.cu
)

cuda_add_library(voxel_extract ${VOCOMP_EXTRACT_SRC})

vm_target_dependency(voxel_extract VMUtils PUBLIC)
vm_target_dependency(voxel_extract cudafx PUBLIC)
target_link_libraries(voxel_extract ${CUDA_CUDA_LIBRARY})

if(MSVC)
  if(CMAKE_CL_64)
    target_link_libraries(voxel_extract
      ${PROJECT_SOURCE_DIR}/lib/x64/nvcuvid.lib
    )
  else()
    target_link_libraries(voxel_extract
      ${PROJECT_SOURCE_DIR}/lib/Win32/nvcuvid.lib
    )
  endif()
else()
  target_link_libraries(voxel_extract
    ${PROJECT_SOURCE_DIR}/lib/linux/stubs/x86_64/libnvcuvid.so
  )
endif()

target_include_directories(voxel_extract PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)