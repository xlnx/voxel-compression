add_executable(voprod voprod.cc)
vm_target_dependency(voprod vocodec PRIVATE)

add_executable(vodecomp vodecomp.cc)
vm_target_dependency(codecomp vocodec PRIVATE)
