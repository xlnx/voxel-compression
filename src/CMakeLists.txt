if (VOCOMP_BUILD_EXTRACTOR)
  add_subdirectory(extract)
endif()

if (VOCOMP_BUILD_REFINER)
  add_subdirectory(refine)
endif()
