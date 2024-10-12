file(GLOB_RECURSE "util_source" CONFIGURE_DEPENDS
    src/*.cpp
)

add_library("util" STATIC ${util_source})
target_include_directories("util" PUBLIC include)
set_property(TARGET "util" PROPERTY CXX_STANDARD 20)
