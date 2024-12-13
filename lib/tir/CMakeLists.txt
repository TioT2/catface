file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(tir STATIC ${source})

# setup include directories
target_include_directories(tir PUBLIC include)

# link dependencies
target_link_libraries(tir PUBLIC util)
target_link_libraries(tir PUBLIC ast)
target_link_libraries(tir PRIVATE m)
