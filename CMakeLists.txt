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

option(VOCOMP_BUILD_NVCODEC "build nvcodec with cuda" ON)
option(VOCOMP_BUILD_REFINER "build refiner library" ON)
option(VOCOMP_BUILD_EXTRACTOR "build extractor library" ON)
option(VOCOMP_BUILD_TOOLS "build tools" ON)

add_subdirectory(src)
if(VOCOMP_BUILD_TOOLS)
  add_subdirectory(tools)
endif()
