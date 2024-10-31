file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_executable(cf_disassembler ${source})

# link dependencies
target_link_libraries(cf_disassembler PRIVATE disassembler)
