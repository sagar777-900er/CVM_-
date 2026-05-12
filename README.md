# CVM++ Language & Virtual Machine

**CVM++** is a custom, dynamically typed scripting language and stack-based bytecode virtual machine written in modern C++ (C++17). Designed from the ground up to be lightweight, modular, and performant, CVM++ features a complete compilation pipeline from source code to executable bytecode.

---

## 🚀 Features

- **Turing Complete**: Full support for variables, arithmetic, logical operators, and control flow (`if`/`else`, `while`).
- **First-Class Functions**: Define reusable blocks of code with `fn`, complete with recursion and parameter passing.
- **Lexical Scoping**: Robust local and global variable tracking. Block scoping behaves intuitively, safely shadowing outer variables.
- **Bytecode Compiler**: A single-pass compiler that generates dense, compact bytecode instructions.
- **High-Performance VM**: A robust, stack-based virtual machine executing the compiled bytecode with an efficient fetch-decode-execute loop.
- **Interactive REPL**: A built-in Read-Eval-Print Loop for testing and experimenting with the language dynamically. Global variables persist across inputs.
- **Professional Debugging Tools**: Granular CLI commands to visualize the compilation pipeline at every step: Lexer Tokens, Abstract Syntax Tree, Disassembled Bytecode, and Real-Time Stack Tracing.

## 🏗️ Architecture Pipeline

The system is broken down into four strict, decoupled phases:

1. **Lexer** (`src/lexer`): Converts raw source code strings into a stream of meaningful tokens.
2. **Parser** (`src/parser`): Uses Recursive Descent and Precedence Climbing to parse tokens into an Abstract Syntax Tree (AST).
3. **Compiler** (`src/compiler`): Walks the AST to emit optimized bytecode chunks and manages the constant pool and symbol tables.
4. **Virtual Machine** (`src/vm`): An efficient stack machine that manages CallFrames and executes the bytecode instructions.

## 🛠️ Build Instructions

### Prerequisites
- A C++17 compatible compiler (e.g., GCC, Clang, MSVC)
- CMake 3.16+

### Compiling from Source

```bash
# Generate build files
cmake -B build -S .

# Compile the project
cmake --build build

# The executable will be located in the build folder:
./build/cvm.exe
```

Alternatively, if you prefer building directly with `g++`:
```bash
g++ -std=c++17 -Isrc src/**/*.cpp -o cvm.exe
```

## 💻 Command-Line Interface

The CVM++ executable provides a rich set of subcommands.

```bash
cvm run <file.cvm>      # Execute a script
cvm repl                # Launch the interactive REPL
cvm trace <file.cvm>    # Execute with real-time stack and instruction tracing
cvm lex <file.cvm>      # Tokenize and print the lexer output
cvm parse <file.cvm>    # Parse and print the Abstract Syntax Tree
cvm compile <file.cvm>  # Compile and print the disassembled bytecode
cvm test                # Run the automated CVM++ test suite
```

*Note: running `cvm <file.cvm>` implicitly uses the `run` subcommand.*

## 📜 Code Examples

### 1. Variables and Arithmetic
```js
let width = 10
let height = 5
let area = width * height
print area  // 50
```

### 2. Control Flow
```js
let count = 5
while count > 0 {
    print count
    count = count - 1
}
print "Liftoff!"
```

### 3. Functions & Recursion
```js
fn factorial(n) {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

print factorial(5) // 120
```

Check out the `/examples` directory for more scripts!

## 🧪 Testing

The CVM++ project includes a built-in test suite to verify the lexer, parser, compiler, and VM behavior.

```bash
cvm test
```

## 📝 License
This project is open-source and intended for educational and portfolio purposes.
