// ============================================================================
// CVM++ : common/pipeline.cpp — Pipeline Driver
// ============================================================================

#include "pipeline.h"

#include <iostream>
#include <iomanip>

#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../compiler/compiler.h"

namespace cvm {

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

void tokenizeAndPrint(const std::string& label, const std::string& source) {
    std::cout << "=== " << label << " ===" << std::endl;
    std::cout << "  Source: \"" << source << "\"" << std::endl;

    cvm::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    printTokens(tokens);
}

void parseAndPrint(const std::string& label, const std::string& source) {
    std::cout << "=== " << label << " ===" << std::endl;
    std::cout << "  Source: \"" << source << "\"" << std::endl;
    std::cout << std::endl;

    cvm::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    cvm::Parser parser(std::move(tokens));
    cvm::Program program = parser.parse();

    if (parser.hadError()) {
        std::cout << "  [Parse failed — see errors above]" << std::endl;
    } else {
        std::cout << cvm::programToString(program);
    }
    std::cout << std::endl;
}

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

void runProgram(const std::string& label, const std::string& source, bool showOutput, bool trace) {
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
    vm.setTraceExecution(trace);
    cvm::VMResult result = vm.interpret(chunk);

    if (result != cvm::VMResult::OK) {
        std::cerr << "  [Runtime error]" << std::endl;
    }

    if (showOutput) {
        std::cout << std::endl;
    }
}

void runTests() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   CVM++ — Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- Lexer Tests ----
    std::cout << std::endl << "--- LEXER TESTS ---" << std::endl << std::endl;
    tokenizeAndPrint("Test 1: Basic Arithmetic", "5 + 2 * 3");
    tokenizeAndPrint("Test 2: Variable Declaration", "let x = 10");
    tokenizeAndPrint("Test 3: String Literal", "let name = \"hello world\"");

    // ---- Parser Tests ----
    std::cout << std::endl << "--- PARSER TESTS ---" << std::endl << std::endl;
    parseAndPrint("Parse 1: Arithmetic", "5 + 2 * 3");
    parseAndPrint("Parse 2: Variable Declaration", "let x = 10");
    parseAndPrint("Parse 3: Functions", "fn add(a, b) { return a + b }");

    // ---- Compiler Tests ----
    std::cout << std::endl << "--- COMPILER TESTS ---" << std::endl << std::endl;
    compileAndPrint("Compile 1: Arithmetic", "5 + 2 * 3");
    compileAndPrint("Compile 2: Print", "print 42");
    compileAndPrint("Compile 3: Functions", "fn add(a, b) { return a + b }");

    // ---- VM Execution Tests ----
    std::cout << std::endl << "--- VM EXECUTION TESTS ---" << std::endl << std::endl;
    runProgram("Run 1: Print constant", "print 42");
    runProgram("Run 2: Arithmetic", "print 5 + 2 * 3");
    runProgram("Run 3: Variables", "let x = 10\nlet y = 20\nprint x + y");
    runProgram("Run 4: Functions", "fn add(a, b) { return a + b }\nprint add(10, 20)");

    std::cout << "========================================" << std::endl;
    std::cout << "   All tests completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
}

} // namespace cvm
