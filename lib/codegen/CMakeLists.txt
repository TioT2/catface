file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(codegen_cfvm STATIC ${source})

# setup include directories
target_include_directories(codegen_cfvm PUBLIC include)

# link dependencies
target_link_libraries(codegen_cfvm PUBLIC util)
target_link_libraries(codegen_cfvm PUBLIC tir)
target_link_libraries(codegen_cfvm PUBLIC object)
target_link_libraries(codegen_cfvm PUBLIC executable)
