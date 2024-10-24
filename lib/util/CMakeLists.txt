file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library(util STATIC ${source})
target_include_directories(util PUBLIC include)
set_property(TARGET util PROPERTY CXX_STANDARD 20)

# link standard math library
target_link_libraries(util m)
