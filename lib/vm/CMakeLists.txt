file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(vm STATIC ${source})

# setup include directories
target_include_directories(vm PUBLIC include)

# link dependencies
target_link_libraries(vm PUBLIC util)
target_link_libraries(vm PUBLIC executable)
target_link_libraries(vm PRIVATE m)
