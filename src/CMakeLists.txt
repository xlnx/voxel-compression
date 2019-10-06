add_subdirectory(video)

if(VOCOMP_BUILD_REFINER)
  add_subdirectory(refine)
endif()

if(VOCOMP_BUILD_EXTRACTOR)
  add_subdirectory(extract)
endif()
