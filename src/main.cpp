// ============================================================================
// CVM++ : main.cpp — Complete Pipeline
// ============================================================================
// Entry point for the CVM++ language: Lexer + Parser + Compiler + VM.
//
// USAGE:
//   cvm <filename>.cvm      → RUN the program (full pipeline)
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
#include <vector>
#include <iomanip>
#include <cstring>

#include "lexer/token.h"
#include "lexer/lexer.h"
#include "ast/ast.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "vm/vm.h"
#include "repl/repl.h"

// ============================================================================
// printTokens — display a formatted token table
// ============================================================================

void printTokens(const std::vector<cvm::Token>& tokens) {
    std::cout << std::endl;
    std::cout << "  " << std::left
              << std::setw(6)  << "Line"
              << std::setw(18) << "Type"
              << "Lexeme" << std::endl;
    std::cout << "  " << std::string(6, '-')
              << std::string(18, '-')
              << std::string(20, '-') << std::endl;

    for (const auto& token : tokens) {
        std::string displayLexeme = token.lexeme;
        if (token.type == cvm::TokenType::NEWLINE) {
            displayLexeme = "<newline>";
        }

        std::cout << "  " << std::left
                  << std::setw(6)  << token.line
                  << std::setw(18) << cvm::tokenTypeToString(token.type)
                  << displayLexeme << std::endl;
    }

    std::cout << std::endl;
    int meaningful = 0;
    for (const auto& t : tokens) {
        if (t.type != cvm::TokenType::NEWLINE && t.type != cvm::TokenType::EOF_TOKEN) {
            meaningful++;
        }
    }
    std::cout << "  Total: " << tokens.size() << " tokens ("
              << meaningful << " meaningful)" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// tokenizeAndPrint — helper that tokenizes source code and prints results
// ============================================================================

void tokenizeAndPrint(const std::string& label, const std::string& source) {
    std::cout << "=== " << label << " ===" << std::endl;
    std::cout << "  Source: \"" << source << "\"" << std::endl;

    cvm::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    printTokens(tokens);
}

// ============================================================================
// parseAndPrint — tokenize, parse, and print the AST
// ============================================================================

void parseAndPrint(const std::string& label, const std::string& source) {
    std::cout << "=== " << label << " ===" << std::endl;
    std::cout << "  Source: \"" << source << "\"" << std::endl;
    std::cout << std::endl;

    // Step 1: Tokenize
    cvm::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    // Step 2: Parse
    cvm::Parser parser(std::move(tokens));
    cvm::Program program = parser.parse();

    if (parser.hadError()) {
        std::cout << "  [Parse failed — see errors above]" << std::endl;
    } else {
        // Step 3: Print the AST
        std::cout << cvm::programToString(program);
    }
    std::cout << std::endl;
}

// ============================================================================
// compileAndPrint — tokenize, parse, compile, and show bytecode
// ============================================================================

void compileAndPrint(const std::string& label, const std::string& source) {
    std::cout << "=== " << label << " ===" << std::endl;
    std::cout << "  Source: \"" << source << "\"" << std::endl;
    std::cout << std::endl;

    cvm::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    cvm::Parser parser(std::move(tokens));
    cvm::Program program = parser.parse();

    if (parser.hadError()) {
        std::cout << "  [Parse failed]" << std::endl;
        return;
    }

    cvm::Compiler compiler;
    cvm::Chunk chunk = compiler.compile(program);

    if (compiler.hadError()) {
        std::cout << "  [Compilation failed]" << std::endl;
        return;
    }

    chunk.disassemble(label);
}

// ============================================================================
// runProgram — full pipeline: lex + parse + compile + execute
// ============================================================================

void runProgram(const std::string& label, const std::string& source,
                bool showOutput = true) {
    if (showOutput) {
        std::cout << "=== " << label << " ===" << std::endl;
    }

    cvm::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    cvm::Parser parser(std::move(tokens));
    cvm::Program program = parser.parse();

    if (parser.hadError()) {
        std::cerr << "  [Parse failed]" << std::endl;
        return;
    }

    cvm::Compiler compiler;
    cvm::Chunk chunk = compiler.compile(program);

    if (compiler.hadError()) {
        std::cerr << "  [Compilation failed]" << std::endl;
        return;
    }

    cvm::VM vm;
    cvm::VMResult result = vm.interpret(chunk);

    if (result != cvm::VMResult::OK) {
        std::cerr << "  [Runtime error]" << std::endl;
    }

    if (showOutput) {
        std::cout << std::endl;
    }
}

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
// runTests — comprehensive test suite for Lexer + Parser
// ============================================================================

void runTests() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   CVM++ — Test Suite (Lexer + Parser)" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- Lexer Tests ----
    std::cout << std::endl << "--- LEXER TESTS ---" << std::endl << std::endl;

    tokenizeAndPrint("Test 1: Basic Arithmetic", "5 + 2 * 3");
    tokenizeAndPrint("Test 2: Variable Declaration", "let x = 10");
    tokenizeAndPrint("Test 3: Comparisons", "x == 5");
    tokenizeAndPrint("Test 4: Bang-Equal", "x != 10");
    tokenizeAndPrint("Test 5: Keywords vs Identifiers", "let letter = 42");
    tokenizeAndPrint("Test 6: Booleans", "true false");
    tokenizeAndPrint("Test 7: Multi-line",
                     "let x = 10\nlet y = 20\nprint x + y");
    tokenizeAndPrint("Test 8: While Loop",
                     "while x < 5 {\n    print x\n}");
    tokenizeAndPrint("Test 9: If-Else",
                     "if x > 0 {\n    print x\n} else {\n    print 0\n}");
    tokenizeAndPrint("Test 10: Comments",
                     "let x = 5 // this is a comment\nprint x");
    tokenizeAndPrint("Test 11: Unknown Characters", "let x = @#$");
    tokenizeAndPrint("Test 12: Empty Input", "");
    tokenizeAndPrint("Test 13: Input Keyword", "let x = input");
    tokenizeAndPrint("Test 14: Parentheses", "(5 + 2) * 3");
    tokenizeAndPrint("Test 15: String Literal", "let name = \"hello world\"");
    tokenizeAndPrint("Test 16: Less/Greater Equal", "x <= 10 and y >= 5");

    // ---- Parser Tests ----
    std::cout << std::endl << "--- PARSER TESTS ---" << std::endl << std::endl;

    parseAndPrint("Parse 1: Arithmetic", "5 + 2 * 3");
    parseAndPrint("Parse 2: Precedence", "(5 + 2) * 3");
    parseAndPrint("Parse 3: Variable Declaration", "let x = 10");
    parseAndPrint("Parse 4: Print", "print x + 1");
    parseAndPrint("Parse 5: Multi-line",
                  "let x = 10\nlet y = 20\nprint x + y");
    parseAndPrint("Parse 6: Unary", "-5 + !true");
    parseAndPrint("Parse 7: Assignment", "x = 42");
    parseAndPrint("Parse 8: Comparison", "x == 5 and y != 10");
    parseAndPrint("Parse 9: If-Else",
                  "if x > 0 {\n  print x\n} else {\n  print 0\n}");
    parseAndPrint("Parse 10: While Loop",
                  "while x < 10 {\n  print x\n  x = x + 1\n}");
    parseAndPrint("Parse 11: Nested Expressions", "a + b * c - d / e");
    parseAndPrint("Parse 12: Boolean Operators", "true and false or true");
    parseAndPrint("Parse 13: Input", "let x = input");
    parseAndPrint("Parse 14: String", "print \"hello world\"");

    // ---- Compiler Tests ----
    std::cout << std::endl << "--- COMPILER TESTS ---" << std::endl << std::endl;

    compileAndPrint("Compile 1: Arithmetic", "5 + 2 * 3");
    compileAndPrint("Compile 2: Variable Decl", "let x = 10");
    compileAndPrint("Compile 3: Print", "print 42");
    compileAndPrint("Compile 4: Multi-line",
                    "let x = 10\nlet y = 20\nprint x + y");
    compileAndPrint("Compile 5: If-Else",
                    "if 1 {\n  print 42\n} else {\n  print 0\n}");
    compileAndPrint("Compile 6: While Loop",
                    "let x = 0\nwhile x < 3 {\n  print x\n  x = x + 1\n}");

    // ---- VM Execution Tests ----
    std::cout << std::endl << "--- VM EXECUTION TESTS ---" << std::endl << std::endl;

    runProgram("Run 1: Print constant", "print 42");
    runProgram("Run 2: Arithmetic", "print 5 + 2 * 3");
    runProgram("Run 3: Variables",
               "let x = 10\nlet y = 20\nprint x + y");
    runProgram("Run 4: If-true",
               "if 1 {\n  print 42\n} else {\n  print 0\n}");
    runProgram("Run 5: If-false",
               "if 0 {\n  print 42\n} else {\n  print 99\n}");
    runProgram("Run 6: While loop",
               "let x = 0\nwhile x < 5 {\n  print x\n  x = x + 1\n}");
    runProgram("Run 7: Comparison",
               "let x = 10\nif x > 5 {\n  print 1\n} else {\n  print 0\n}");
    runProgram("Run 8: String print", "print \"Hello, CVM++!\"");
    runProgram("Run 9: Negation", "print -42");
    runProgram("Run 10: Boolean logic", "print !false");

    std::cout << "========================================" << std::endl;
    std::cout << "   All tests completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
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

    // --test: run comprehensive test suite
    if (std::strcmp(argv[1], "--test") == 0) {
        runTests();
        return 0;
    }

    // --repl: launch interactive REPL
    if (std::strcmp(argv[1], "--repl") == 0) {
        cvm::REPL repl;
        repl.run();
        return 0;
    }

    // --lex <file>: show tokens only
    if (std::strcmp(argv[1], "--lex") == 0) {
        if (argc < 3) {
            std::cerr << "[Error] --lex requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        tokenizeAndPrint("Tokens: " + std::string(argv[2]), source);
        return 0;
    }

    // --parse <file>: show AST only
    if (std::strcmp(argv[1], "--parse") == 0) {
        if (argc < 3) {
            std::cerr << "[Error] --parse requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        parseAndPrint("AST: " + std::string(argv[2]), source);
        return 0;
    }

    // --compile <file>: show disassembled bytecode
    if (std::strcmp(argv[1], "--compile") == 0) {
        if (argc < 3) {
            std::cerr << "[Error] --compile requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        compileAndPrint("Bytecode: " + std::string(argv[2]), source);
        return 0;
    }

    // --run <file>: execute the program (explicit flag)
    if (std::strcmp(argv[1], "--run") == 0) {
        if (argc < 3) {
            std::cerr << "[Error] --run requires a filename." << std::endl;
            return 1;
        }
        std::string source = readFile(argv[2]);
        runProgram("Running: " + std::string(argv[2]), source);
        return 0;
    }

    // File mode — RUN the program (default action for .cvm files)
    std::string filename = argv[1];
    std::string source = readFile(filename);
    if (source.empty() && filename.find(".cvm") == std::string::npos) {
        std::cerr << "[Error] File appears empty or could not be read." << std::endl;
        return 1;
    }

    std::cout << std::endl;
    runProgram("Running: " + filename, source);

    return 0;
}
