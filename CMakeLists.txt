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

option(VOCOMP_ENABLE "enable compression" ON)
option(VOCOMP_ENABLE_CUDA "enable cuda compression" ON)
option(VOCOMP_ENABLE_GL "enable gl compression" ON)
option(VOCOMP_ENABLE_D3D9 "enable d3d9 compression" ON)

if(VOCOMP_ENABLE)
  add_definitions(-DVOCOMP_ENABLE)
endif()

if(VOCOMP_ENABLE_CUDA AND VOCOMP_ENABLE AND CUDA_FOUND)
  set(VOCOMP_ENABLE_CUDA ON)
else()
  set(VOCOMP_ENABLE_CUDA OFF)
endif()
if(VOCOMP_ENABLE_GL AND VOCOMP_ENABLE AND UNIX AND NOT APPLE)
  set(VOCOMP_ENABLE_GL ON)
else()
  set(VOCOMP_ENABLE_GL OFF)
endif()
if(VOCOMP_ENABLE_D3D9 AND VOCOMP_ENABLE AND WIN32)
  set(VOCOMP_ENABLE_D3D9 ON)
else()
  set(VOCOMP_ENABLE_D3D9 OFF)
endif()

if(VOCOMP_ENABLE_CUDA)
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
