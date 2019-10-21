if(NOT VOCOMP_ENABLE_CUDA)
  message(FATAL_ERROR "video_decode library requires Cuda support, which is not found")
endif()

file(GLOB_RECURSE VIDEO_DECOMPRESS_SRC
  *.cc
  **/*.cc
  **/*.cu
)

vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cuda-fx
  GIT_TAG        master
)

cuda_add_library(video_decompress ${VIDEO_DECOMPRESS_SRC})
target_link_libraries(video_decompress ${CUDA_CUDA_LIBRARY})
if(MSVC)
  if(CMAKE_CL_64)
    target_link_libraries(video_decompress
      ${PROJECT_SOURCE_DIR}/lib/x64/nvcuvid.lib
    )
  else()
    target_link_libraries(video_decompress
      ${PROJECT_SOURCE_DIR}/lib/Win32/nvcuvid.lib
    )
  endif()
else()
  target_link_libraries(video_decompress
    ${PROJECT_SOURCE_DIR}/lib/linux/stubs/x86_64/libnvcuvid.so
  )
endif()
vm_target_dependency(video_decompress cudafx PRIVATE)

target_include_directories(video_decompress PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
