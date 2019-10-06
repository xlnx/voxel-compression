set(CMAKE_CXX_STANDARD 14)

add_library(voextract extractor.cc)
target_include_directories(voextract PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
vm_target_dependency(voextract vocomp PUBLIC)
