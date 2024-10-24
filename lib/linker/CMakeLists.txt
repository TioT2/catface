file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library(linker STATIC ${source})
target_include_directories(linker PUBLIC include)
set_property(TARGET linker PROPERTY CXX_STANDARD 20)

target_link_libraries(linker PUBLIC util)
target_link_libraries(linker PUBLIC object)
target_link_libraries(linker PUBLIC executable)
