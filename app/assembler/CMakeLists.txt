file(GLOB_RECURSE "app_assembler_source" CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable("cf_assembler" ${app_assembler_source})
set_property(TARGET "cf_assembler" PROPERTY CXX_STANDARD 20)

target_link_libraries("cf_assembler" LINK_PRIVATE asm)
target_link_libraries("cf_assembler" LINK_PRIVATE util)