# find necessary packages
find_package(SDL2 REQUIRED)


file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})


add_executable("cf_executor" ${source})
set_property(TARGET "cf_executor" PROPERTY CXX_STANDARD 20)

# link dependencies
target_link_libraries("cf_executor" PRIVATE vm)
target_link_libraries("cf_executor" PRIVATE SDL2)
