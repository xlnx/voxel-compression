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

file(GLOB_RECURSE VOCOMP_SOURCES
  src/*.cc
  src/*.cu
)

find_package(CUDA REQUIRED)

cuda_add_executable(vocomp ${VOCOMP_SOURCES})
target_include_directories(vocomp PUBLIC
  include
  nvcodec/include
  nvcodec/Samples/External/FFmpeg/include
  ${CUDA_INCLUDE_DIRS}
)
file(GLOB_RECURSE NVCODEC_LIBS
  nvcodec/lib/linux/stubs/x86_64/*
)
target_link_libraries(vocomp
  avutil
  avcodec
  avformat
  ${NVCODEC_LIBS}
  cuda
)
vm_target_dependency(vocomp VMUtils PUBLIC)
vm_target_dependency(vocomp cudafx PUBLIC)
