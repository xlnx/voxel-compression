# add_executable(voprod voprod.cc)

if(VOCOMP_BUILD_REFINER)
  add_executable(voxel-refine voxel-refine.cc)
  if(VOCOMP_BUILD_VIDEO_COMPRESS)
	vm_target_dependency(voxel-refine video_compress PRIVATE)
  endif()
  vm_target_dependency(voxel-refine voxel_refine PRIVATE)
endif()
