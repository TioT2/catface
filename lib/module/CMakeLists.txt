file(GLOB_RECURSE "src" CONFIGURE_DEPENDS
    src/*.c
)

set_source_files_properties(${source} include/*.h PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library("module" STATIC ${src})
target_include_directories("module" PUBLIC include)
set_property(TARGET "module" PROPERTY CXX_STANDARD 20)

target_link_libraries("module" PUBLIC util)
