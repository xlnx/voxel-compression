cmake_minimum_required(VERSION 3.9)
project(voxel-compression)

find_package(Git)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive)
# install external modules
include(scripts/external.cmake)
vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/VMUtils
  GIT_TAG        master
)
vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cuda-fx
  GIT_TAG        master
)

file(GLOB_RECURSE VOCODEC_SOURCES
  src/*.cc
  src/**/*.cc
)

find_package(CUDA REQUIRED)

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

cuda_add_library(vocodec ${VOCODEC_SOURCES})
target_include_directories(vocodec PUBLIC
  include
  ${FFMPEG_INCLUDE_DIR}
  ${CUDA_INCLUDE_DIRS}
)
target_include_directories(vocodec PRIVATE
  nvcodec/include
)
file(GLOB_RECURSE NVCODEC_LIBS
  nvcodec/lib/linux/stubs/x86_64/*
)
target_link_libraries(vocodec
  ${AVUTIL_LIB}
  ${AVCODEC_LIB}
  ${AVFORMAT_LIB}
  ${NVCODEC_LIBS}
)
vm_target_dependency(vocodec VMUtils PUBLIC)
vm_target_dependency(vocodec cudafx PUBLIC)

add_subdirectory(tools)
