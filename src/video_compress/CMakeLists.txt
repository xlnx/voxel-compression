file(GLOB_RECURSE VIDEO_COMPRESS_SRC
  *.cc
  **/*.cc
  **/*.cu
)

if(VOCOMP_ENABLE_CUDA)
  vm_external_module(
    GIT_REPOSITORY https://github.com/cad420/cuda-fx
    GIT_TAG        master
  )

  cuda_add_library(video_compress ${VIDEO_COMPRESS_SRC})
  target_link_libraries(video_compress ${CUDA_CUDA_LIBRARY})
  # target_include_directories(vocomp PRIVATE
  #   ${CUDA_INCLUDE_DIRS}
  # )
  vm_target_dependency(video_compress cudafx PRIVATE)
else()
  add_library(video_compress ${VIDEO_COMPRESS_SRC})
endif()
vm_target_dependency(video_compress VMUtils PUBLIC)

find_path(FFMPEG_INCLUDE_DIR
  NAMES ffmpeg/libavcodec/avcodec.h
  PATHS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
)
target_include_directories(video_compress PRIVATE
  ${FFMPEG_INCLUDE_DIR}/ffmpeg
)
if(UNIX AND NOT APPLE)
  find_library(AVUTIL_LIB
    NAMES avutil
    PATHS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}
  )
  find_library(AVCODEC_LIB
    NAMES avcodec
    PATHS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}
  )
  find_library(AVFORMAT_LIB
    NAMES avformat
    PATHS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}
  )
  target_link_libraries(video_compress
    ${AVUTIL_LIB}
    ${AVCODEC_LIB}
    ${AVFORMAT_LIB}
  )
endif()

if(MSVC)
  if(CMAKE_CL_64)
    target_link_libraries(video_compress
      ${PROJECT_SOURCE_DIR}/lib/x64/nvencodeapi.lib
    )
  else()
    target_link_libraries(video_compress
      ${PROJECT_SOURCE_DIR}/lib/Win32/nvencodeapi.lib
    )
  endif()
else()
  target_link_libraries(video_compress
  	${PROJECT_SOURCE_DIR}/lib/linux/stubs/x86_64/libnvidia-encode.so
  )
endif()

if(VOCOMP_ENABLE_D3D9)
  target_link_libraries(video_compress d3d9)
endif()

if(VOCOMP_ENABLE_GL)
  find_package(GLEW REQUIRED)
  target_include_directories(video_compress PRIVATE ${GLEW_INCLUDE_DIRS})
  target_link_libraries(video_compress GL ${GLEW_LIBRARIES})
endif()

target_include_directories(video_compress PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
