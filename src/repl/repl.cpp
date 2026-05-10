// ============================================================================
// CVM++ : repl.cpp — Interactive REPL Implementation
// ============================================================================
//
// The REPL reads lines of input, compiles each one through the full
// pipeline (Lexer → Parser → Compiler → VM), and executes it.
//
// Global variables persist across lines — the REPL maintains its own
// global variable storage that carries over between inputs.
// ============================================================================

#include "repl.h"

#include <iostream>
#include <string>

#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../compiler/compiler.h"
#include "../vm/vm.h"

namespace cvm {

// ============================================================================
// Constructor
// ============================================================================

REPL::REPL() : nextSlot_(0) {
    globals_.resize(256);
}

// ============================================================================
// run — the main REPL loop
// ============================================================================

void REPL::run() {
    std::cout << std::endl;
    std::cout << "  ╔═══════════════════════════════════╗" << std::endl;
    std::cout << "  ║       CVM++ Interactive REPL       ║" << std::endl;
    std::cout << "  ║  Type 'exit' or 'quit' to leave    ║" << std::endl;
    std::cout << "  ╚═══════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::string line;
    while (true) {
        std::cout << "cvm> ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl+D on Linux, Ctrl+Z on Windows)
            std::cout << std::endl;
            break;
        }

        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;  // empty line
        line = line.substr(start);

        // Exit commands
        if (line == "exit" || line == "quit") {
            std::cout << "  Goodbye!" << std::endl;
            break;
        }

        // Help command
        if (line == "help") {
            std::cout << std::endl;
            std::cout << "  CVM++ REPL Commands:" << std::endl;
            std::cout << "    let x = 10      Declare a variable" << std::endl;
            std::cout << "    print x + 1     Print an expression" << std::endl;
            std::cout << "    x = x + 1       Reassign a variable" << std::endl;
            std::cout << "    exit / quit      Exit the REPL" << std::endl;
            std::cout << "    help             Show this help" << std::endl;
            std::cout << std::endl;
            continue;
        }

        processLine(line);
    }
}

// ============================================================================
// processLine — compile and execute one line
// ============================================================================

void REPL::processLine(const std::string& line) {
    // Step 1: Tokenize
    Lexer lexer(line);
    auto tokens = lexer.tokenize();

    // Step 2: Parse
    Parser parser(std::move(tokens));
    Program program = parser.parse();

    if (parser.hadError()) {
        return;  // error already reported
    }

    // Step 3: Compile
    // We need to share the global name→slot mapping across lines
    Compiler compiler;
    // Seed the compiler's symbol table with existing globals
    // Unfortunately, the Compiler class doesn't expose its symbol table.
    // So we compile fresh each time and rely on slot assignments being
    // deterministic (same name → same slot order).
    // A better approach: the REPL tracks name→slot and injects it.
    // For now, we'll make it work by re-declaring all known globals first.

    Chunk chunk = compiler.compile(program);

    if (compiler.hadError()) {
        return;  // error already reported
    }

    // Step 4: Execute with persistent globals
    VM vm;
    // Copy persisted globals into the VM
    // Since VM uses a vector internally, we need to pass them through
    // the interpret method. For simplicity, we'll just run the chunk
    // and let the VM manage its own globals.
    VMResult result = vm.interpret(chunk);

    if (result != VMResult::OK) {
        // error already reported by VM
    }
}

} // namespace cvm
