add_executable(test_ast main.cpp)
target_link_libraries(test_ast PRIVATE ast)
target_compile_definitions(test_ast PRIVATE _TEST_EXAMPLE_DIR="${CMAKE_SOURCE_DIR}/examples/lang")