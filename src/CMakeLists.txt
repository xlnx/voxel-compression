set(VMCORE_SHARED_LIBRARY OFF)
vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/VMCore
  GIT_TAG        dev/flingza
)

if (VARCH_BUILD_UNARCHIVER)
  add_subdirectory(unarchive)
endif()

if (VARCH_BUILD_ARCHIVER)
  add_subdirectory(archive)
endif()
