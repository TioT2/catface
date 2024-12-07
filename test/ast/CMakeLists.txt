file(READ example.cf content)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/gen/example.cf" "R\"(\n${content}\n)\"")


add_executable(test_ast main.cpp)
include_directories(test_ast PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test_ast PRIVATE ast)
