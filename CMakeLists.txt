cmake_minimum_required(VERSION 3.14)

project("catface")

# compile libraries
add_subdirectory(lib/util)
add_subdirectory(lib/module)
add_subdirectory(lib/asm)
add_subdirectory(lib/vm)

# compile applications
add_subdirectory(app/vm)
add_subdirectory(app/assembler)
add_subdirectory(app/disassembler)
