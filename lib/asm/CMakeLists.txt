
file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library("asm" STATIC ${source})
target_include_directories("asm" PUBLIC include)
set_property(TARGET "asm" PROPERTY CXX_STANDARD 20)

target_link_libraries("asm" PUBLIC util)
target_link_libraries("asm" PUBLIC module)
