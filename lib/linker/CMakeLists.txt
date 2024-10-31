file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(linker STATIC ${source})

# setup include directories
target_include_directories(linker PUBLIC include)

# link dependencies
target_link_libraries(linker PUBLIC util)
target_link_libraries(linker PUBLIC object)
target_link_libraries(linker PUBLIC executable)
