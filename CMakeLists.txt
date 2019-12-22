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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

option(VOCOMP_BUILD_REFINER "build refiner library" ON)
option(VOCOMP_BUILD_EXTRACTOR "build extractor library" ON)
option(VOCOMP_BUILD_TOOLS "build tools" ON)
option(VOCOMP_BUILD_TESTS "build tests" OFF)

add_subdirectory(src)

if(VOCOMP_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(VOCOMP_BUILD_TESTS)
  add_subdirectory(tests)
endif()
