file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(executable STATIC ${source})

# setup include directories
target_include_directories(executable PUBLIC include)

# link dependencies
target_link_libraries(executable PUBLIC util)
