file(GLOB_RECURSE "app_vm_source" CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable("cf_vm" ${app_vm_source})
target_link_libraries("cf_vm" PUBLIC vm)

set_property(TARGET "cf_vm" PROPERTY CXX_STANDARD 20)
