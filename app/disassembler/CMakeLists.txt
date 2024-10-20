file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_executable("cf_disassembler" ${source})
set_property(TARGET "cf_disassembler" PROPERTY CXX_STANDARD 20)

target_link_libraries("cf_disassembler" PRIVATE disassembler)
