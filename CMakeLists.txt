cmake_minimum_required(VERSION 3.9)
project(voxel-compression)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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

option(VARCH_BUILD_ARCHIVER "build archiver library" ON)
option(VARCH_BUILD_UNARCHIVER "build unarchiver library" ON)
option(VARCH_BUILD_TOOLS "build tools" ON)
option(VARCH_BUILD_TESTS "build tests" OFF)
option(VARCH_ENABLE_OPENH264 "enable openh264 for codec" ON)

if (VARCH_ENABLE_OPENH264)
  find_package(OpenH264)
  if (OPENH264_FOUND)
    add_definitions(-DVARCH_OPENH264_CODEC)
  endif()
else()
  set(OPENH264_FOUND OFF)
endif()

add_subdirectory(src)

if(VARCH_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(VARCH_BUILD_TESTS)
  add_subdirectory(tests)
endif()
