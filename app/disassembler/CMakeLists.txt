file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable("cf_disassembler" ${source})
set_property(TARGET "cf_disassembler" PROPERTY CXX_STANDARD 20)

target_link_libraries("cf_disassembler" PUBLIC asm)
