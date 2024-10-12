file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable("cf_executor" ${source})
set_property(TARGET "cf_executor" PROPERTY CXX_STANDARD 20)

target_link_libraries("cf_executor" PUBLIC vm)
target_link_libraries("cf_executor" PUBLIC module)
