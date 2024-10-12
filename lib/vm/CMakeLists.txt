file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library("vm" STATIC ${source})
target_include_directories("vm" PUBLIC include)
set_property(TARGET "vm" PROPERTY CXX_STANDARD 20)

target_link_libraries("vm" PUBLIC util)
target_link_libraries("vm" PUBLIC module)
