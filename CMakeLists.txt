cmake_minimum_required(VERSION 3.14)

project(catface)

# add cmake-specific flag to disable 'C with C++ compiler' deprecation warning
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-deprecated")
endif()

# set compiler language
set(CF_LANGUAGE CXX)

# set preferred C++ standard
set(CMAKE_CXX_STANDARD 20)

# compile libraries
add_subdirectory(lib/util)
add_subdirectory(lib/executable)
add_subdirectory(lib/object)
add_subdirectory(lib/assembler)
add_subdirectory(lib/disassembler)
add_subdirectory(lib/linker)
add_subdirectory(lib/vm)

# add implementation librariess
add_subdirectory(impl/sandbox_sdl2)

# compile applications
add_subdirectory(app/executor)
add_subdirectory(app/assembler)
add_subdirectory(app/disassembler)
add_subdirectory(app/linker)

# compile tests (debug-only)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_subdirectory(test/list)
    add_subdirectory(test/list_dot_dump)
endif()
