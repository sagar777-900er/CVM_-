// ============================================================================
// CVM++ : parser.cpp — Parser Implementation
// ============================================================================
//
// This file implements the recursive descent parser.  Each parsing function
// corresponds to a grammar rule and calls lower-level functions for
// higher-precedence operations.
//
// CALL HIERARCHY (expression parsing):
//
//   parseExpression()               <- entry point (handles assignment)
//     |-- parseOr()                 <- logical OR
//         |-- parseAnd()            <- logical AND
//             |-- parseEquality()       <- == !=
//                 |-- parseComparison() <- < > <= >=
//                     |-- parseAddition()    <- + -
//                         |-- parseMultiplication()  <- * /
//                             |-- parseUnary()   <- - !
//                                 |-- parsePrimary()  <- numbers, idents, (expr)
//
// Each level handles operators of one precedence level, then delegates to
// the next level for tighter-binding operators.  This naturally produces
// a tree where higher-precedence operators are deeper (evaluated first).
// ============================================================================

#include "parser.h"
#include <iostream>
#include <utility>

namespace cvm {

// ============================================================================
// Constructor
// ============================================================================

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), current_(0), hadError_(false) {}

// ============================================================================
// parse — main entry point
// ============================================================================

Program Parser::parse() {
    Program program;

    skipNewlines();

    while (!isAtEnd()) {
        program.statements.push_back(parseStatement());
        skipNewlines();
    }

    return program;
}

// ============================================================================
// STATEMENT PARSERS
// ============================================================================

Stmt Parser::parseStatement() {
    if (match(TokenType::LET))   return parseLetStatement();
    if (match(TokenType::PRINT)) return parsePrintStatement();
    if (match(TokenType::IF))    return parseIfStatement();
    if (match(TokenType::WHILE)) return parseWhileStatement();
    return parseExpressionStatement();
}

// parseLetStatement — "let" IDENTIFIER "=" expression NEWLINE
Stmt Parser::parseLetStatement() {
    const Token& name = consume(TokenType::IDENTIFIER,
                                "Expected variable name after 'let'.");
    consume(TokenType::EQUAL, "Expected '=' after variable name in let statement.");
    ExprPtr initializer = parseExpression();
    expectNewline();

    return makeLetStmt(name.lexeme, std::move(initializer));
}

// parsePrintStatement — "print" expression NEWLINE
Stmt Parser::parsePrintStatement() {
    ExprPtr expr = parseExpression();
    expectNewline();

    return makePrintStmt(std::move(expr));
}

// parseIfStatement — "if" expression block ("else" block)?
Stmt Parser::parseIfStatement() {
    ExprPtr condition = parseExpression();
    skipNewlines();

    std::vector<Stmt> thenBranch = parseBlock();

    std::vector<Stmt> elseBranch;
    skipNewlines();
    if (match(TokenType::ELSE)) {
        skipNewlines();
        elseBranch = parseBlock();
    }

    return makeIfStmt(std::move(condition), std::move(thenBranch),
                      std::move(elseBranch));
}

// parseWhileStatement — "while" expression block
Stmt Parser::parseWhileStatement() {
    ExprPtr condition = parseExpression();
    skipNewlines();

    std::vector<Stmt> body = parseBlock();

    return makeWhileStmt(std::move(condition), std::move(body));
}

// parseExpressionStatement — expression NEWLINE
Stmt Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();
    expectNewline();

    return makeExpressionStmt(std::move(expr));
}

// parseBlock — "{" NEWLINE? statement* "}"
std::vector<Stmt> Parser::parseBlock() {
    consume(TokenType::LBRACE, "Expected '{' to begin block.");
    skipNewlines();

    std::vector<Stmt> statements;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(parseStatement());
        skipNewlines();
    }

    consume(TokenType::RBRACE, "Expected '}' to end block.");
    return statements;
}

// ============================================================================
// EXPRESSION PARSERS
// ============================================================================

// parseExpression — handles assignment (lowest precedence, right-associative)
ExprPtr Parser::parseExpression() {
    ExprPtr expr = parseOr();

    if (match(TokenType::EQUAL)) {
        ExprPtr value = parseExpression();  // right-associative: recurse

        // The left side must be an identifier for assignment
        if (expr->type == ExprType::IDENTIFIER) {
            return makeAssign(expr->name, std::move(value));
        }

        error("Invalid assignment target.");
    }

    return expr;
}

// parseOr — expr ("or" expr)*
ExprPtr Parser::parseOr() {
    ExprPtr left = parseAnd();

    while (match(TokenType::OR)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseAnd();
        left = makeBinary(op, std::move(left), std::move(right));
    }

    return left;
}

// parseAnd — expr ("and" expr)*
ExprPtr Parser::parseAnd() {
    ExprPtr left = parseEquality();

    while (match(TokenType::AND)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseEquality();
        left = makeBinary(op, std::move(left), std::move(right));
    }

    return left;
}

// parseEquality — expr ("==" | "!=") expr
ExprPtr Parser::parseEquality() {
    ExprPtr left = parseComparison();

    while (match(TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseComparison();
        left = makeBinary(op, std::move(left), std::move(right));
    }

    return left;
}

// parseComparison — expr ("<" | ">" | "<=" | ">=") expr
ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddition();

    while (match(TokenType::LESS, TokenType::GREATER,
                 TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseAddition();
        left = makeBinary(op, std::move(left), std::move(right));
    }

    return left;
}

// parseAddition — expr ("+" | "-") expr
ExprPtr Parser::parseAddition() {
    ExprPtr left = parseMultiplication();

    while (match(TokenType::PLUS, TokenType::MINUS)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseMultiplication();
        left = makeBinary(op, std::move(left), std::move(right));
    }

    return left;
}

// parseMultiplication — expr ("*" | "/") expr
ExprPtr Parser::parseMultiplication() {
    ExprPtr left = parseUnary();

    while (match(TokenType::STAR, TokenType::SLASH)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseUnary();
        left = makeBinary(op, std::move(left), std::move(right));
    }

    return left;
}

// parseUnary — ("-" | "!") expr | primary
ExprPtr Parser::parseUnary() {
    if (match(TokenType::MINUS, TokenType::BANG)) {
        std::string op = previous().lexeme;
        ExprPtr operand = parseUnary();  // recursive: allows --x, !!flag
        return makeUnary(op, std::move(operand));
    }

    return parsePrimary();
}

// parsePrimary — literals, identifiers, parenthesized expressions
ExprPtr Parser::parsePrimary() {
    if (match(TokenType::NUMBER)) {
        return makeNumberLiteral(std::stoi(previous().lexeme));
    }

    if (match(TokenType::STRING)) {
        return makeStringLiteral(previous().lexeme);
    }

    if (match(TokenType::TRUE_LITERAL)) {
        return makeBoolLiteral(true);
    }

    if (match(TokenType::FALSE_LITERAL)) {
        return makeBoolLiteral(false);
    }

    if (match(TokenType::INPUT)) {
        return makeInput();
    }

    if (match(TokenType::IDENTIFIER)) {
        return makeIdentifier(previous().lexeme);
    }

    if (match(TokenType::LPAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return expr;
    }

    // Nothing matched — error
    error("Expected an expression.");
    return makeNumberLiteral(0);  // dummy node for recovery
}

// ============================================================================
// TOKEN HELPERS
// ============================================================================

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(message);
    return peek();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

void Parser::error(const std::string& message) {
    error(peek(), message);
}

void Parser::error(const Token& token, const std::string& message) {
    hadError_ = true;
    std::cerr << "[Parse Error] Line " << token.line << ": " << message;
    if (token.type == TokenType::EOF_TOKEN) {
        std::cerr << " (at end of file)";
    } else {
        std::cerr << " (at '" << token.lexeme << "')";
    }
    std::cerr << std::endl;
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == TokenType::NEWLINE) return;

        switch (peek().type) {
            case TokenType::LET:
            case TokenType::PRINT:
            case TokenType::IF:
            case TokenType::WHILE:
                return;
            default:
                break;
        }

        advance();
    }
}

// ============================================================================
// NEWLINE HANDLING
// ============================================================================

void Parser::skipNewlines() {
    while (match(TokenType::NEWLINE) || match(TokenType::SEMICOLON)) {
        // consume all newlines and semicolons
    }
}

void Parser::expectNewline() {
    if (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON)) {
        advance();
        return;
    }
    if (isAtEnd() || check(TokenType::RBRACE)) {
        return;  // EOF or closing brace ends a statement implicitly
    }
    error("Expected newline or ';' after statement.");
}

} // namespace cvm
