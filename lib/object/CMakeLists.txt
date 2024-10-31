file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_library(object STATIC ${source})

# setup include directories
target_include_directories(object PUBLIC include)

target_link_libraries(object PUBLIC util)