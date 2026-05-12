// ============================================================================
// CVM++ : parser/parser.cpp — Parser Implementation
// ============================================================================
// Recursive descent parser. Each parsing function corresponds to a grammar
// rule and calls lower-level functions for higher-precedence operations.
// ============================================================================

#include "parser.h"
#include <iostream>
#include <utility>

namespace cvm {

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
    if (match(TokenType::LET))    return parseLetStatement();
    if (match(TokenType::PRINT))  return parsePrintStatement();
    if (match(TokenType::IF))     return parseIfStatement();
    if (match(TokenType::WHILE))  return parseWhileStatement();
    if (match(TokenType::FN))     return parseFunctionStatement();
    if (match(TokenType::RETURN)) return parseReturnStatement();

    // Block statement: standalone { ... }
    if (match(TokenType::LBRACE)) {
        // We already consumed the '{', so parse the block body directly
        skipNewlines();
        std::vector<Stmt> statements;
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            statements.push_back(parseStatement());
            skipNewlines();
        }
        consume(TokenType::RBRACE, "Expected '}' to end block.");
        return makeBlockStmt(std::move(statements));
    }

    return parseExpressionStatement();
}

// let IDENTIFIER = expression NEWLINE
Stmt Parser::parseLetStatement() {
    const Token& name = consume(TokenType::IDENTIFIER,
                                "Expected variable name after 'let'.");
    consume(TokenType::EQUAL, "Expected '=' after variable name.");
    ExprPtr initializer = parseExpression();
    expectNewline();
    return makeLetStmt(name.lexeme, std::move(initializer));
}

// print expression NEWLINE
Stmt Parser::parsePrintStatement() {
    ExprPtr expr = parseExpression();
    expectNewline();
    return makePrintStmt(std::move(expr));
}

// if expression block ("else" block)?
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

// while expression block
Stmt Parser::parseWhileStatement() {
    ExprPtr condition = parseExpression();
    skipNewlines();
    std::vector<Stmt> body = parseBlock();
    return makeWhileStmt(std::move(condition), std::move(body));
}

// fn IDENTIFIER "(" params? ")" block
Stmt Parser::parseFunctionStatement() {
    const Token& name = consume(TokenType::IDENTIFIER,
                                "Expected function name after 'fn'.");
    consume(TokenType::LPAREN, "Expected '(' after function name.");

    std::vector<std::string> params;
    if (!check(TokenType::RPAREN)) {
        do {
            const Token& param = consume(TokenType::IDENTIFIER,
                                         "Expected parameter name.");
            params.push_back(param.lexeme);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters.");

    skipNewlines();
    std::vector<Stmt> body = parseBlock();

    return makeFunctionStmt(name.lexeme, std::move(params), std::move(body));
}

// return expression? NEWLINE
Stmt Parser::parseReturnStatement() {
    ExprPtr expr = nullptr;
    // If there's an expression on the same line, parse it
    if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) &&
        !check(TokenType::RBRACE) && !isAtEnd()) {
        expr = parseExpression();
    }
    expectNewline();
    return makeReturnStmt(std::move(expr));
}

// expression NEWLINE
Stmt Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();
    expectNewline();
    return makeExpressionStmt(std::move(expr));
}

// "{" NEWLINE? statement* "}"
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

// Assignment (lowest precedence, right-associative)
ExprPtr Parser::parseExpression() {
    ExprPtr expr = parseOr();

    if (match(TokenType::EQUAL)) {
        ExprPtr value = parseExpression();
        if (expr->type == ExprType::IDENTIFIER) {
            return makeAssign(expr->name, std::move(value));
        }
        error("Invalid assignment target.");
    }

    return expr;
}

ExprPtr Parser::parseOr() {
    ExprPtr left = parseAnd();
    while (match(TokenType::OR)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseAnd();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseAnd() {
    ExprPtr left = parseEquality();
    while (match(TokenType::AND)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseEquality();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseEquality() {
    ExprPtr left = parseComparison();
    while (match(TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseComparison();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

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

ExprPtr Parser::parseAddition() {
    ExprPtr left = parseMultiplication();
    while (match(TokenType::PLUS, TokenType::MINUS)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseMultiplication();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseMultiplication() {
    ExprPtr left = parseUnary();
    while (match(TokenType::STAR, TokenType::SLASH)) {
        std::string op = previous().lexeme;
        ExprPtr right = parseUnary();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (match(TokenType::MINUS, TokenType::BANG)) {
        std::string op = previous().lexeme;
        ExprPtr operand = parseUnary();
        return makeUnary(op, std::move(operand));
    }
    return parseCall();
}

// Function call: identifier "(" args? ")"
ExprPtr Parser::parseCall() {
    ExprPtr expr = parsePrimary();

    // If the primary was an identifier followed by '(', it's a call
    if (expr->type == ExprType::IDENTIFIER && match(TokenType::LPAREN)) {
        std::string callee = expr->name;
        std::vector<ExprPtr> args;

        if (!check(TokenType::RPAREN)) {
            do {
                args.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }

        consume(TokenType::RPAREN, "Expected ')' after arguments.");
        return makeCall(callee, std::move(args));
    }

    return expr;
}

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

    error("Expected an expression.");
    return makeNumberLiteral(0);
}

// ============================================================================
// TOKEN HELPERS
// ============================================================================

const Token& Parser::peek() const     { return tokens_[current_]; }
const Token& Parser::previous() const { return tokens_[current_ - 1]; }

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
    std::cerr << "[line " << token.line << "] Syntax Error: " << message;
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
            case TokenType::FN:
            case TokenType::RETURN:
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
    while (match(TokenType::NEWLINE) || match(TokenType::SEMICOLON)) {}
}

void Parser::expectNewline() {
    if (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON)) {
        advance();
        return;
    }
    if (isAtEnd() || check(TokenType::RBRACE)) {
        return;
    }
    error("Expected newline or ';' after statement.");
}

} // namespace cvm
