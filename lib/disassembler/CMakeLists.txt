file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(disassembler STATIC ${source})

# setup include directories
target_include_directories(disassembler PUBLIC include)

# link dependencies
target_link_libraries(disassembler PUBLIC util)
target_link_libraries(disassembler PUBLIC executable)
