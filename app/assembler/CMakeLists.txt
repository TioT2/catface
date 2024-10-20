file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_executable("cf_assembler" ${source})
set_property(TARGET "cf_assembler" PROPERTY CXX_STANDARD 20)

target_link_libraries("cf_assembler" PRIVATE assembler)
target_link_libraries("cf_assembler" PRIVATE linker)
target_link_libraries("cf_assembler" PRIVATE util)