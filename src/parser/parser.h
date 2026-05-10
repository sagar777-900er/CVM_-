// ============================================================================
// CVM++ : parser.h — Parser Class Declaration
// ============================================================================
//
// The Parser is the second stage of the compiler pipeline.  It takes the
// flat token stream from the Lexer and builds an AST (Abstract Syntax Tree)
// that represents the hierarchical structure of the program.
//
// PARSING TECHNIQUE
// -----------------
// We use a **Recursive Descent** parser with precedence climbing for
// expressions.  This is the technique used by GCC, Clang, V8, and most
// hand-written production parsers.
//
//   - Each grammar rule becomes a function (hence "recursive descent")
//   - Operator precedence is handled by calling functions in order
//   - Higher precedence = tighter binding = evaluated first
//
// PRECEDENCE TABLE (low to high)
// --------------------------------
//   1. Assignment:     =          (right-associative)
//   2. Or:             or
//   3. And:            and
//   4. Equality:       ==  !=
//   5. Comparison:     <  >  <=  >=
//   6. Addition:       +  -
//   7. Multiplication: *  /
//   8. Unary:          -  !      (prefix operators)
//   9. Primary:        literals, identifiers, (expr)
//
// STATEMENT GRAMMAR
// -----------------
//   program     -> statement* EOF
//   statement   -> letStmt | printStmt | ifStmt | whileStmt | exprStmt
//   letStmt     -> "let" IDENTIFIER "=" expression NEWLINE
//   printStmt   -> "print" expression NEWLINE
//   ifStmt      -> "if" expression block ("else" block)?
//   whileStmt   -> "while" expression block
//   exprStmt    -> expression NEWLINE
//   block       -> "{" NEWLINE? statement* "}"
// ============================================================================

#ifndef CVM_PARSER_H
#define CVM_PARSER_H

#include <vector>
#include <string>

#include "../ast/ast.h"
#include "../lexer/token.h"

namespace cvm {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // parse — main entry point. Returns a Program AST.
    Program parse();

    // hadError — check if any parse errors occurred
    bool hadError() const { return hadError_; }

private:
    // ======================== STATEMENT PARSERS ============================

    Stmt parseStatement();
    Stmt parseLetStatement();
    Stmt parsePrintStatement();
    Stmt parseIfStatement();
    Stmt parseWhileStatement();
    Stmt parseExpressionStatement();
    std::vector<Stmt> parseBlock();

    // ======================== EXPRESSION PARSERS ===========================
    // Ordered from lowest to highest precedence.

    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAddition();
    ExprPtr parseMultiplication();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();

    // ======================== TOKEN HELPERS =================================

    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);

    // Variadic match — check multiple types
    template <typename... Types>
    bool match(TokenType first, Types... rest) {
        if (match(first)) return true;
        return match(rest...);
    }

    const Token& consume(TokenType type, const std::string& message);
    bool isAtEnd() const;

    // ======================== ERROR HANDLING ================================

    void error(const std::string& message);
    void error(const Token& token, const std::string& message);
    void synchronize();

    // ======================== NEWLINE HANDLING ==============================

    void skipNewlines();
    void expectNewline();

    // ============================ DATA =====================================

    std::vector<Token> tokens_;
    size_t             current_;
    bool               hadError_;
};

} // namespace cvm

#endif // CVM_PARSER_H
