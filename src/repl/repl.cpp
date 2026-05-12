// ============================================================================
// CVM++ : repl/repl.cpp — Interactive REPL Implementation
// ============================================================================

#include "repl.h"

#include <iostream>
#include <string>

#include "../lexer/lexer.h"
#include "../parser/parser.h"

namespace cvm {

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
            std::cout << std::endl;
            break;
        }

        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line == "exit" || line == "quit") {
            std::cout << "  Goodbye!" << std::endl;
            break;
        }

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

void REPL::processLine(const std::string& line) {
    Lexer lexer(line);
    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parse();

    if (parser.hadError()) {
        return;
    }

    // Pass false to keep global symbol mappings persistent across lines
    Chunk chunk = compiler_.compile(program, false);

    if (compiler_.hadError()) {
        return;
    }

    // VM uses the same globals vector across executions
    VMResult result = vm_.interpret(chunk);

    if (result != VMResult::OK) {
        // VM already printed the runtime error
    }
}

} // namespace cvm
