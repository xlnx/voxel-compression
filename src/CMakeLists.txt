if (VARCH_BUILD_UNARCHIVER)
  add_subdirectory(unarchive)
endif()

if (VARCH_BUILD_ARCHIVER)
  add_subdirectory(archive)
endif()
