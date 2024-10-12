
file(GLOB_RECURSE "asm_source" CONFIGURE_DEPENDS
    src/*.cpp
)

add_library("asm" STATIC ${asm_source})
target_include_directories("asm" PUBLIC include)
set_property(TARGET "asm" PROPERTY CXX_STANDARD 20)

target_link_libraries("asm" PUBLIC util)
target_link_libraries("asm" PUBLIC module)
