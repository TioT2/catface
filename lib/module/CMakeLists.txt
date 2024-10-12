file(GLOB_RECURSE "src" CONFIGURE_DEPENDS
    src/*.cpp
)

add_library("module" STATIC ${src})
target_include_directories("module" PUBLIC include)
set_property(TARGET "module" PROPERTY CXX_STANDARD 20)

target_link_libraries("module" PUBLIC util)