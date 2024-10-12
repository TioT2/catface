file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable("cf_vm" ${source})
set_property(TARGET "cf_vm" PROPERTY CXX_STANDARD 20)

target_link_libraries("cf_vm" PUBLIC vm)
