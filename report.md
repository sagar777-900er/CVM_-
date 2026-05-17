# CVM++ Project Report

## 1. Introduction
CVM++ is a custom stack-based virtual machine and compiler written in modern C++ (C++17). The project demonstrates the complete compilation pipeline from lexical analysis to runtime execution. It was designed from scratch to be lightweight, performant, and fully capable of executing a custom, dynamically-typed scripting language.

## 2. Architecture
The architecture of CVM++ is broken down into four distinct, highly decoupled phases. The compilation pipeline operates linearly:
1. **Source Code** is read from a file or REPL input.
2. **Lexer** tokenizes the raw source code string into meaningful tokens.
3. **Parser** processes the tokens and generates an Abstract Syntax Tree (AST).
4. **Compiler** walks the AST and emits bytecode.
5. **Virtual Machine** executes the bytecode instructions.

## 3. Core Components
- **Lexer (`src/lexer/`)**: Uses a single-pass scanner to group characters into tokens (e.g., `TOKEN_IDENTIFIER`, `TOKEN_NUMBER`, `TOKEN_PLUS`). It also tracks line numbers for error reporting.
- **Parser (`src/parser/`)**: Employs Recursive Descent for statements and Precedence Climbing for mathematical expressions. It produces a structured AST that perfectly represents the execution order of the program.
- **Compiler (`src/compiler/`)**: Traverses the AST in a single pass to generate a `Chunk` of bytecode. It handles symbol resolution, determining variable offsets, and translating high-level constructs into low-level instructions.
- **Virtual Machine (`src/vm/`)**: An efficient stack-based execution engine. It uses a `CallFrame` architecture to manage function calls, local variables, and the execution of the instruction pointer (`ip`).
- **REPL (`src/repl/`)**: An interactive Read-Eval-Print Loop that allows users to type CVM++ code and see the execution results instantly, with global state persisting across lines.

## 4. Bytecode Design
The compiler targets a custom instruction set. Bytecode operations (opcodes) are 8-bit integers that instruct the VM on what to do. Example opcodes include:
- `OP_CONSTANT`: Loads a constant value from the chunk's constant pool onto the stack.
- `OP_ADD`, `OP_MULTIPLY`: Pops two values off the stack, performs the arithmetic, and pushes the result back.
- `OP_STORE`: Pops a value and stores it into a variable.
- `OP_LOAD`: Pushes the value of a variable onto the stack.
- `OP_PRINT`: Pops a value and prints it to standard output.
- `OP_JUMP`, `OP_JUMP_IF_FALSE`: Modifies the instruction pointer for control flow (`if` and `while` loops).

## 5. Stack-Based Execution
The Virtual Machine is "stack-based", meaning there are no registers. All operations push and pop values from a central data stack. 
For example, to execute `print 10 + 20`:
1. The VM executes `OP_CONSTANT 10`, pushing `10` onto the stack: `[10]`
2. The VM executes `OP_CONSTANT 20`, pushing `20` onto the stack: `[10, 20]`
3. The VM executes `OP_ADD`, popping `20` and `10`, adding them, and pushing `30`: `[30]`
4. The VM executes `OP_PRINT`, popping `30` and printing it. Stack becomes empty: `[]`

## 6. Supported Features
- **Variables**: Dynamically typed local and global variables (`let`).
- **Control Flow**: Conditional `if/else` statements and `while` loops.
- **Functions**: First-class functions (`fn`) with support for recursion and return values.
- **Lexical Scoping**: Block scopes `{}` that allow safe variable shadowing and resolution.
- **Debugging Tools**: Granular CLI commands to visualize tokens (`cvm lex`), the AST (`cvm parse`), disassembled bytecode (`cvm compile`), and a real-time stack trace (`cvm trace`).

## 7. Challenges Faced
Building a compiler and virtual machine from scratch presented several complex engineering challenges:
- **Parsing Complexity**: Implementing robust precedence climbing for mathematical expressions required careful management of operator rules to ensure `a + b * c` parsed correctly as `a + (b * c)`.
- **Scope Handling**: Ensuring that variables resolved to the correct lexical scope (especially inside nested blocks and recursive functions) required a meticulous symbol table architecture in the compiler.
- **VM Execution and Stack Frames**: Managing the call stack during function execution, allocating local variables at relative stack offsets, and cleaning up the stack on `return` was difficult to get perfectly right without memory leaks or stack underflows.
- **Debugging**: Bytecode bugs are notoriously hard to track down. I solved this by building the `trace` debugging command, which visualizes the stack and instructions in real-time, proving invaluable during development.

## 8. Future Improvements
While CVM++ is highly capable, future iterations could include:
- **Garbage Collection (GC)**: Implementing a Mark-and-Sweep garbage collector to manage heap-allocated objects like arrays or classes.
- **Advanced Data Types**: Adding native support for Strings, Arrays, and Hash Maps.
- **Optimizations**: Implementing a JIT (Just-In-Time) compiler or adding peephole optimizations to the bytecode compiler to speed up execution times.
