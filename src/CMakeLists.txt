if(VOCOMP_BUILD_VIDEO_COMPRESS)
  add_subdirectory(video_compress)
endif()

if(VOCOMP_BUILD_VIDEO_DECOMPRESS)
  add_subdirectory(video_decompress)
endif()
  
if(VOCOMP_BUILD_REFINER)
  add_subdirectory(refine)
endif()

if(VOCOMP_BUILD_EXTRACTOR)
  add_subdirectory(extract)
endif()
