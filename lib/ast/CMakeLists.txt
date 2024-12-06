file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)

set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(ast STATIC ${source})

# setup include directories
target_include_directories(ast PUBLIC include)

# link dependencies
target_link_libraries(ast PUBLIC util)
target_link_libraries(ast PRIVATE m)
