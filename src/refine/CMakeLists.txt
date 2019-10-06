set(CMAKE_CXX_STANDARD 17)

vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/VMCore
  GIT_TAG        master
)

add_library(vorefine refiner.cc)
if (UNIX)
  target_link_libraries(vorefine pthread dl)
endif()
target_include_directories(vorefine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
vm_target_dependency(vorefine vocomp PUBLIC)
vm_target_dependency(vorefine vmcore PUBLIC)
target_link_libraries(vorefine vmcore)
