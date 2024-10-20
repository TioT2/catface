file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library("object" STATIC ${source})
target_include_directories("object" PUBLIC include)
set_property(TARGET "object" PROPERTY CXX_STANDARD 20)
