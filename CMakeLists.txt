# ===========================================================================
# CVM++ Build System
# ===========================================================================

cmake_minimum_required(VERSION 3.16)

project(CVM++ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===========================================================================
# Source files
# ===========================================================================

set(CVM_SOURCES
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/ast/ast.cpp
    src/compiler/compiler.cpp
    src/vm/vm.cpp
    src/repl/repl.cpp
    src/common/pipeline.cpp
)

# ===========================================================================
# Executable
# ===========================================================================

add_executable(cvm ${CVM_SOURCES})

target_include_directories(cvm PRIVATE ${CMAKE_SOURCE_DIR}/src)

# ===========================================================================
# Compiler Warnings
# ===========================================================================

if(MSVC)
    target_compile_options(cvm PRIVATE /W4)
else()
    target_compile_options(cvm PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()
