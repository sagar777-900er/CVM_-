// ============================================================================
// CVM++ : token.h
// ============================================================================
// This file defines the fundamental building blocks of the Lexer:
//
//   1. TokenType  — An enum that lists every kind of token the language knows.
//   2. Token      — A lightweight struct that pairs a TokenType with the
//                   actual text (lexeme) found in the source code and the
//                   line number where it appeared.
//   3. Helper     — tokenTypeToString() for pretty-printing during debugging.
//
// WHY THIS FILE EXISTS
// --------------------
// The rest of the compiler (Parser, Compiler, VM) never looks at raw
// characters.  They work exclusively with Tokens.  This file is the
// "contract" that every later stage depends on.
//
// DESIGN DECISIONS
// ----------------
// * enum class (scoped enum) prevents name collisions and is type-safe.
//   You can't accidentally compare a TokenType with a plain int.
// * Token is a struct, not a class, because it is a plain data carrier
//   with no invariants to protect.  All fields are public.
// * We store the lexeme as a std::string (a copy of the source text).
//   A production compiler might use std::string_view to avoid copies,
//   but owning strings is simpler and safer for a learning project.
// ============================================================================

#ifndef CVM_TOKEN_H
#define CVM_TOKEN_H

#include <string>

namespace cvm {

// ============================================================================
// TokenType — every distinct kind of token the language supports
// ============================================================================
//
// CATEGORIES:
//   Literals    — NUMBER, TRUE_LITERAL, FALSE_LITERAL
//   Identifiers — IDENTIFIER (variable / function names)
//   Operators   — PLUS, MINUS, STAR, SLASH, EQUAL, EQUAL_EQUAL, etc.
//   Delimiters  — LPAREN, RPAREN, LBRACE, RBRACE
//   Keywords    — LET, PRINT, IF, ELSE, WHILE, INPUT
//   Special     — NEWLINE, EOF_TOKEN, UNKNOWN
//
// NOTE: We call it EOF_TOKEN (not EOF) because EOF is already a macro
//       defined by <cstdio>.  Reusing that name would cause conflicts.
// ============================================================================

enum class TokenType {
    // ---- Literals ----
    NUMBER,             // e.g. 42, 0, 999
    TRUE_LITERAL,       // true
    FALSE_LITERAL,      // false

    // ---- Identifiers ----
    IDENTIFIER,         // e.g. x, myVar, count

    // ---- Literals (continued) ----
    STRING,             // e.g. "hello world"

    // ---- Arithmetic Operators ----
    PLUS,               // +
    MINUS,              // -
    STAR,               // *
    SLASH,              // /

    // ---- Comparison / Logical Operators ----
    EQUAL,              // =   (assignment)
    EQUAL_EQUAL,        // ==  (equality test)
    BANG,               // !   (logical NOT)
    BANG_EQUAL,         // !=  (not-equal test)
    LESS,               // <
    LESS_EQUAL,         // <=  (less-or-equal)
    GREATER,            // >
    GREATER_EQUAL,      // >=  (greater-or-equal)

    // ---- Delimiters ----
    LPAREN,             // (
    RPAREN,             // )
    LBRACE,             // {
    RBRACE,             // }
    SEMICOLON,          // ;   (optional statement separator)
    COMMA,              // ,   (future: function arguments)

    // ---- Keywords ----
    LET,                // let
    PRINT,              // print
    IF,                 // if
    ELSE,               // else
    WHILE,              // while
    INPUT,              // input
    AND,                // and  (logical AND)
    OR,                 // or   (logical OR)
    FN,                 // fn   (future: function declaration)
    RETURN,             // return (future: function return)

    // ---- Special ----
    NEWLINE,            // \n  — used as a statement separator
    EOF_TOKEN,          // signals end-of-input
    UNKNOWN             // anything the lexer doesn't recognize
};

// ============================================================================
// tokenTypeToString — convert a TokenType to a human-readable label
// ============================================================================
// This is used for debug output.  Every time we add a new TokenType above,
// we must add a matching case here.
//
// We implement it inline in the header so that every translation unit that
// includes token.h can call it without needing a separate .cpp file.
// ============================================================================

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::NUMBER:         return "NUMBER";
        case TokenType::STRING:         return "STRING";
        case TokenType::TRUE_LITERAL:   return "TRUE";
        case TokenType::FALSE_LITERAL:  return "FALSE";
        case TokenType::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::PLUS:           return "PLUS";
        case TokenType::MINUS:          return "MINUS";
        case TokenType::STAR:           return "STAR";
        case TokenType::SLASH:          return "SLASH";
        case TokenType::EQUAL:          return "EQUAL";
        case TokenType::EQUAL_EQUAL:    return "EQUAL_EQUAL";
        case TokenType::BANG:           return "BANG";
        case TokenType::BANG_EQUAL:     return "BANG_EQUAL";
        case TokenType::LESS:           return "LESS";
        case TokenType::LESS_EQUAL:     return "LESS_EQUAL";
        case TokenType::GREATER:        return "GREATER";
        case TokenType::GREATER_EQUAL:  return "GREATER_EQUAL";
        case TokenType::LPAREN:         return "LPAREN";
        case TokenType::RPAREN:         return "RPAREN";
        case TokenType::LBRACE:         return "LBRACE";
        case TokenType::RBRACE:         return "RBRACE";
        case TokenType::SEMICOLON:      return "SEMICOLON";
        case TokenType::COMMA:          return "COMMA";
        case TokenType::LET:            return "LET";
        case TokenType::PRINT:          return "PRINT";
        case TokenType::IF:             return "IF";
        case TokenType::ELSE:           return "ELSE";
        case TokenType::WHILE:          return "WHILE";
        case TokenType::INPUT:          return "INPUT";
        case TokenType::AND:            return "AND";
        case TokenType::OR:             return "OR";
        case TokenType::FN:             return "FN";
        case TokenType::RETURN:         return "RETURN";
        case TokenType::NEWLINE:        return "NEWLINE";
        case TokenType::EOF_TOKEN:      return "EOF";
        case TokenType::UNKNOWN:        return "UNKNOWN";
        default:                        return "???";
    }
}

// ============================================================================
// Token — a single token produced by the Lexer
// ============================================================================
//
// Fields:
//   type   — what kind of token this is (from the enum above)
//   lexeme — the actual characters from the source code (e.g. "42", "let", "+")
//   line   — which source line the token appeared on (1-based)
//
// The line number is critical for error reporting in later phases.
// When the Parser finds a syntax error, it can tell the user exactly
// which line to look at.
// ============================================================================

struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;

    // Constructor for convenient token creation
    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), line(line) {}
};

} // namespace cvm

#endif // CVM_TOKEN_H
