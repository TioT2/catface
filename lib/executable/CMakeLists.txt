file(GLOB_RECURSE "src" CONFIGURE_DEPENDS
    src/*.c
)

set_source_files_properties(${source} include/*.h PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library("executable" STATIC ${src})
target_include_directories("executable" PUBLIC include)
set_property(TARGET "executable" PROPERTY CXX_STANDARD 20)

target_link_libraries("executable" PUBLIC util)
