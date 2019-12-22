# add_executable(voprod voprod.cc)

if(VOCOMP_BUILD_REFINER)
  cuda_add_executable(voxel-refine voxel-refine.cc)
  vm_target_dependency(voxel-refine voxel_refine PRIVATE)
endif()

if(VOCOMP_BUILD_EXTRACTOR)
  cuda_add_executable(voxel-info voxel-info.cc)
  vm_target_dependency(voxel-info voxel_extract PRIVATE)
endif()
