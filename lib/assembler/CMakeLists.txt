file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library(assembler STATIC ${source})
target_include_directories(assembler PUBLIC include)
set_property(TARGET assembler PROPERTY CXX_STANDARD 20)

target_link_libraries(assembler PUBLIC util)
target_link_libraries(assembler PUBLIC object)
target_link_libraries(assembler PUBLIC executable)
