# ===========================================================================
# CVM++ Build System
# ===========================================================================
# This CMakeLists.txt builds the CVM++ compiler/VM project.
#
# HOW TO BUILD:
#   cmake -B build -S .
#   cmake --build build
#
# HOW TO RUN:
#   .\build\Debug\cvm.exe                        (default demo)
#   .\build\Debug\cvm.exe --test                  (test suite)
#   .\build\Debug\cvm.exe examples\hello.cvm      (tokenize a file)
# ===========================================================================

cmake_minimum_required(VERSION 3.16)

# Project name, version, and language
project(CVM++ VERSION 0.1.0 LANGUAGES CXX)

# Use C++17 standard — we need:
#   - std::string_view (later phases)
#   - structured bindings
#   - std::optional
#   - if-init statements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Don't use compiler-specific extensions

# ===========================================================================
# Source files
# ===========================================================================
# We list all .cpp files explicitly rather than using GLOB.
# This is CMake best practice because GLOB doesn't re-run CMake when
# new files are added, which can cause confusing build failures.
# ===========================================================================

set(CVM_SOURCES
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/compiler/compiler.cpp
    src/vm/vm.cpp
    src/repl/repl.cpp
)

# ===========================================================================
# Executable
# ===========================================================================

add_executable(cvm ${CVM_SOURCES})

# Add src/ to the include path so we can write:
#   #include "lexer/token.h"
# instead of:
#   #include "../lexer/token.h"
target_include_directories(cvm PRIVATE ${CMAKE_SOURCE_DIR}/src)

# ===========================================================================
# Compiler Warnings
# ===========================================================================
# Enable strict warnings to catch bugs early.
# Different compilers use different flags.
# ===========================================================================

if(MSVC)
    # MSVC (Visual Studio) — /W4 enables most useful warnings
    target_compile_options(cvm PRIVATE /W4)
else()
    # GCC / Clang — enable all warnings + extra warnings
    target_compile_options(cvm PRIVATE -Wall -Wextra -Wpedantic)
endif()
