# add_executable(voprod voprod.cc)

if(VARCH_BUILD_ARCHIVER)
  cuda_add_executable(voxel-archive voxel-archive.cc)
  vm_target_dependency(voxel-archive voxel_archive PRIVATE)
endif()

if(VARCH_BUILD_UNARCHIVER)
  cuda_add_executable(voxel-info voxel-info.cc)
  vm_target_dependency(voxel-info voxel_unarchive PRIVATE)
endif()
