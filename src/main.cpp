// ============================================================================
// CVM++ : main.cpp — Complete Pipeline
// ============================================================================
// Entry point for the CVM++ language.
//
// USAGE:
//   cvm <filename>.cvm      → RUN the program
//   cvm                     → launch interactive REPL
//   cvm --test              → runs comprehensive test suite
//   cvm --lex <file>        → show tokens only
//   cvm --parse <file>      → show AST only
//   cvm --compile <file>    → show disassembled bytecode
//   cvm --run <file>        → run the program (explicit flag)
//   cvm --repl              → launch interactive REPL (explicit flag)
// ============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

#include "repl/repl.h"
#include "common/pipeline.h"

// ============================================================================
// readFile — read the entire contents of a file into a string
// ============================================================================

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Error] Could not open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ============================================================================
// main — entry point
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << std::endl;
    std::cout << "  ╔═══════════════════════════════════╗" << std::endl;
    std::cout << "  ║       CVM++ Language v1.0         ║" << std::endl;
    std::cout << "  ║   Stack-Based VM & Compiler        ║" << std::endl;
    std::cout << "  ╚═══════════════════════════════════╝" << std::endl;

    if (argc < 2) {
        // No arguments — launch REPL
        cvm::REPL repl;
        repl.run();
        return 0;
    }

    std::string command = argv[1];

    if (command == "test") {
        cvm::runTests();
        return 0;
    }

    if (command == "repl") {
        cvm::REPL repl;
        repl.run();
        return 0;
    }

    if (command == "lex") {
        if (argc < 3) {
            std::cerr << "[Error] 'lex' requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        cvm::tokenizeAndPrint("Tokens: " + std::string(argv[2]), source);
        return 0;
    }

    if (command == "parse") {
        if (argc < 3) {
            std::cerr << "[Error] 'parse' requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        cvm::parseAndPrint("AST: " + std::string(argv[2]), source);
        return 0;
    }

    if (command == "compile") {
        if (argc < 3) {
            std::cerr << "[Error] 'compile' requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        cvm::compileAndPrint("Bytecode: " + std::string(argv[2]), source);
        return 0;
    }

    if (command == "trace") {
        if (argc < 3) {
            std::cerr << "[Error] 'trace' requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        cvm::runProgram("Tracing: " + std::string(argv[2]), source, true, true);
        return 0;
    }

    if (command == "run") {
        if (argc < 3) {
            std::cerr << "[Error] 'run' requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        cvm::runProgram("Running: " + std::string(argv[2]), source, true, false);
        return 0;
    }

    // If it's not a known command, assume it's a filename to run directly
    std::string filename = argv[1];
    std::string source = readFile(filename);
    if (source.empty() && filename.find(".cvm") == std::string::npos) {
        std::cerr << "[Error] Unknown command or file could not be read: " << filename << std::endl;
        std::cerr << "Usage: cvm [run|repl|trace|lex|parse|compile|test] <filename>" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    cvm::runProgram("Running: " + filename, source, true, false);

    return 0;
}
