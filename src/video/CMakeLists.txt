set(VOCOMP_SRC
  compressor.cc decompressor.cc
)

if(VOCOMP_BUILD_NVCODEC)
  file(GLOB_RECURSE NVCODEC_SRC
	nv/*.cc
	nv/*.cu
  )
  vm_external_module(
	GIT_REPOSITORY https://github.com/cad420/cuda-fx
	GIT_TAG        master
  )
  find_package(CUDA REQUIRED)
  file(GLOB_RECURSE NVCODEC_LIBS
	${CMAKE_CURRENT_SOURCE_DIR}/../../nvcodec/lib/linux/stubs/x86_64/*
  )

  cuda_add_library(vocomp ${VOCOMP_SRC} ${NVCODEC_SRC})
  target_link_libraries(vocomp ${NVCODEC_LIBS})
  target_include_directories(vocomp PRIVATE
    ${CUDA_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}/../../nvcodec/include
  )
  vm_target_dependency(vocomp cudafx PUBLIC)
else()
  add_library(vocomp ${VOCOMP_SRC})  
endif()

find_path(FFMPEG_INCLUDE_DIR
  NAMES ffmpeg/libavcodec/avcodec.h
  PATHS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
)
set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR}/ffmpeg)
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
target_include_directories(vocomp PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
  ${FFMPEG_INCLUDE_DIR}
)
vm_target_dependency(vocomp VMUtils PUBLIC)

