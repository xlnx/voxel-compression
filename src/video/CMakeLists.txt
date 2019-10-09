file(GLOB_RECURSE VOCOMP_SRC
  compressor.cc
  **/*.cc
  **/*.cu
)

if(VOCOMP_ENABLE_CUDA)
  vm_external_module(
    GIT_REPOSITORY https://github.com/cad420/cuda-fx
    GIT_TAG        master
  )

  cuda_add_library(vocomp ${VOCOMP_SRC})
  target_link_libraries(vocomp ${CUDA_CUDA_LIBRARY})
  if(MSVC)
    if(CMAKE_CL_64)
      target_link_libraries(vocomp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/x64/nvcuvid.lib
      )
    else()
      target_link_libraries(vocomp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/Win32/nvcuvid.lib
      )
    endif()
  else()
    target_link_libraries(vocomp
  	  ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/linux/stubs/x86_64/libnvcuvid.so
    )
  endif()
  # target_include_directories(vocomp PRIVATE
  #   ${CUDA_INCLUDE_DIRS}
  # )
  vm_target_dependency(vocomp cudafx PRIVATE)
else()
  add_library(vocomp ${VOCOMP_SRC})  
endif()

if(MSVC)
  if(CMAKE_CL_64)
    target_link_libraries(vocomp
      ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/x64/nvencodeapi.lib
    )
  else()
    target_link_libraries(vocomp
      ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/Win32/nvencodeapi.lib
    )
  endif()
else()
  target_link_libraries(vocomp
  	${CMAKE_CURRENT_SOURCE_DIR}/../../lib/linux/stubs/x86_64/libnvidia-encode.so
  )
endif()

if(VOCOMP_ENABLE_D3D9)
  target_link_libraries(vocomp d3d9)
endif()

if(VOCOMP_ENABLE_GL)
  find_package(GLEW REQUIRED)
  target_include_directories(vocomp PRIVATE ${GLEW_INCLUDE_DIRS})
  target_link_libraries(vocomp GL ${GLEW_LIBRARIES})
endif()

find_path(FFMPEG_INCLUDE_DIR
  NAMES ffmpeg/libavcodec/avcodec.h
  PATHS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
)
set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR}/ffmpeg)
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
  target_link_libraries(vocomp
    ${AVUTIL_LIB}
    ${AVCODEC_LIB}
    ${AVFORMAT_LIB}
  )
endif()
target_include_directories(vocomp PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
target_include_directories(vocomp PRIVATE
  ${FFMPEG_INCLUDE_DIR}
)
vm_target_dependency(vocomp VMUtils PUBLIC)
