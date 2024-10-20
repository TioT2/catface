cmake_minimum_required(VERSION 3.14)

project(catface)

# add cmake-specific flag to disable 'C with C++ compiler' deprecation warning
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-deprecated")
endif()

# set compiler language
set(CF_LANGUAGE CXX)

# compile libraries
add_subdirectory(lib/util)
add_subdirectory(lib/module)
add_subdirectory(lib/assembler)
add_subdirectory(lib/disassembler)
add_subdirectory(lib/vm)

# compile applications
add_subdirectory(app/executor)
add_subdirectory(app/assembler)
add_subdirectory(app/disassembler)
