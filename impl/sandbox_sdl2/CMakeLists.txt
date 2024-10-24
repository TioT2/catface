# find necessary packages
find_package(SDL2 REQUIRED)

file(GLOB_RECURSE source CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CF_LANGUAGE})

add_library(impl_sandbox_sdl2 ${source})
target_include_directories(impl_sandbox_sdl2 PUBLIC include)
set_property(TARGET impl_sandbox_sdl2 PROPERTY CXX_STANDARD 20)

# link dependencies
target_link_libraries(impl_sandbox_sdl2 PUBLIC vm)
target_link_libraries(impl_sandbox_sdl2 PUBLIC SDL2)
