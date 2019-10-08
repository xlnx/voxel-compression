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

find_package(CUDA)

option(VOCOMP_ENABLE_CUDA "enable cuda compression" ON)
option(VOCOMP_ENABLE_GL "enable gl compression" ON)
option(VOCOMP_ENABLE_D3D9 "enable d3d9 compression" ON)

if(CUDA_FOUND AND VOCOMP_ENABLE_CUDA)
  add_definitions(-DVOCOMP_ENABLE_CUDA)
endif()
if(VOCOMP_ENABLE_GL)
  add_definitions(-DVOCOMP_ENABLE_GL)
endif()
if(VOCOMP_ENABLE_D3D9)
  add_definitions(-DVOCOMP_ENABLE_D3D9)
endif()

option(VOCOMP_BUILD_REFINER "build refiner library" ON)
option(VOCOMP_BUILD_EXTRACTOR "build extractor library" ON)
option(VOCOMP_BUILD_TOOLS "build tools" ON)

add_subdirectory(src)
if(VOCOMP_BUILD_TOOLS)
  add_subdirectory(tools)
endif()
