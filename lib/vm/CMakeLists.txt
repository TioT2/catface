file(GLOB_RECURSE "vm_source"  CONFIGURE_DEPENDS
    src/*.cpp
)

add_library("vm" STATIC ${vm_source})
target_include_directories("vm" PUBLIC include)
set_property(TARGET "vm" PROPERTY CXX_STANDARD 20)

target_link_libraries("vm" PUBLIC util)
target_link_libraries("vm" PUBLIC module)
