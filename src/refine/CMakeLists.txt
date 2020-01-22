set(CMAKE_CXX_STANDARD 17)

set(VMCORE_SHARED_LIBRARY OFF)
vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/VMCore
  GIT_TAG        dev/flingza
)
vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cuda-fx
  GIT_TAG        master
)

file(GLOB_RECURSE VOCOMP_REFINE_SRC
  *.cc
  **/*.cc
  **/*.cu
)

cuda_add_library(voxel_refine ${VOCOMP_REFINE_SRC})

target_link_libraries(voxel_refine ${CUDA_CUDA_LIBRARY})
if (UNIX)
target_link_libraries(voxel_refine pthread dl)
endif()
# target_include_directories(vocomp 
#   ${CUDA_INCLUDE_DIRS}
# )
vm_target_dependency(voxel_refine cudafx PUBLIC)
vm_target_dependency(voxel_refine VMUtils PUBLIC)
vm_target_dependency(voxel_refine vmcore PRIVATE)
if (UNIX)
  target_link_libraries(voxel_refine pthread dl)
endif()
target_link_libraries(voxel_refine vmcore)

if(MSVC)
  if(CMAKE_CL_64)
    target_link_libraries(voxel_refine
      ${PROJECT_SOURCE_DIR}/lib/x64/nvencodeapi.lib
    )
  else()
    target_link_libraries(voxel_refine
      ${PROJECT_SOURCE_DIR}/lib/Win32/nvencodeapi.lib
    )
  endif()
else()
  target_link_libraries(voxel_refine
  	${PROJECT_SOURCE_DIR}/lib/linux/stubs/x86_64/libnvidia-encode.so
  )
endif()

target_include_directories(voxel_refine PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
