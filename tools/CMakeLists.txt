# add_executable(voprod voprod.cc)

if(VOCOMP_BUILD_REFINER)
  add_executable(voxel-refine voxel-refine.cc)
  # add_executable(vodecomp vodecomp.cc)
  vm_target_dependency(voxel-refine vorefine PRIVATE)
endif()
