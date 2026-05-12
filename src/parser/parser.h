// ============================================================================
// CVM++ : parser/parser.h — Parser Class Declaration
// ============================================================================
// Recursive descent parser with precedence climbing for expressions.
//
// PRECEDENCE TABLE (low to high):
//   1. Assignment:     =          (right-associative)
//   2. Or:             or
//   3. And:            and
//   4. Equality:       ==  !=
//   5. Comparison:     <  >  <=  >=
//   6. Addition:       +  -
//   7. Multiplication: *  /
//   8. Unary:          -  !      (prefix)
//   9. Call:           f(x)      (postfix)
//  10. Primary:        literals, identifiers, (expr)
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

    Program parse();
    bool hadError() const { return hadError_; }

private:
    // ======================== STATEMENT PARSERS ============================
    Stmt parseStatement();
    Stmt parseLetStatement();
    Stmt parsePrintStatement();
    Stmt parseIfStatement();
    Stmt parseWhileStatement();
    Stmt parseFunctionStatement();
    Stmt parseReturnStatement();
    Stmt parseExpressionStatement();
    std::vector<Stmt> parseBlock();

    // ======================== EXPRESSION PARSERS ===========================
    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAddition();
    ExprPtr parseMultiplication();
    ExprPtr parseUnary();
    ExprPtr parseCall();
    ExprPtr parsePrimary();

    // ======================== TOKEN HELPERS =================================
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);

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
