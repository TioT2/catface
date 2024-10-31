# find necessary packages

file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})
add_executable(cf_executor ${source})

# link dependencies
target_link_libraries(cf_executor PRIVATE vm)
target_link_libraries(cf_executor PRIVATE impl_sandbox_sdl2)