// ============================================================================
// CVM++ : lexer/lexer.cpp — Lexer Implementation
// ============================================================================

#include "lexer.h"
#include <iostream>

namespace cvm {

// Static keyword lookup table
const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    { "let",    TokenType::LET           },
    { "print",  TokenType::PRINT         },
    { "if",     TokenType::IF            },
    { "else",   TokenType::ELSE          },
    { "while",  TokenType::WHILE         },
    { "true",   TokenType::TRUE_LITERAL  },
    { "false",  TokenType::FALSE_LITERAL },
    { "input",  TokenType::INPUT         },
    { "and",    TokenType::AND           },
    { "or",     TokenType::OR            },
    { "fn",     TokenType::FN            },
    { "return", TokenType::RETURN        },
};

Lexer::Lexer(std::string source)
    : source_(std::move(source)), start_(0), current_(0), line_(1) {}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start_ = current_;
        scanToken();
    }
    tokens_.emplace_back(TokenType::EOF_TOKEN, "", line_);
    return std::move(tokens_);
}

void Lexer::scanToken() {
    char c = advance();

    switch (c) {
        // Single-character tokens
        case '+': addToken(TokenType::PLUS);    break;
        case '-': addToken(TokenType::MINUS);   break;
        case '*': addToken(TokenType::STAR);    break;
        case '/':
            if (match('/')) {
                // Single-line comment — consume until end of line
                while (!isAtEnd() && peek() != '\n') advance();
            } else {
                addToken(TokenType::SLASH);
            }
            break;
        case '(': addToken(TokenType::LPAREN);  break;
        case ')': addToken(TokenType::RPAREN);  break;
        case '{': addToken(TokenType::LBRACE);  break;
        case '}': addToken(TokenType::RBRACE);  break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case ',': addToken(TokenType::COMMA);     break;

        // Two-character tokens
        case '=':
            addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
            break;
        case '!':
            addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
            break;
        case '<':
            addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
            break;
        case '>':
            addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
            break;

        // String literals — now a proper case instead of hiding in default
        case '"':
            scanString();
            break;

        // Whitespace
        case ' ': case '\t': case '\r':
            break;

        // Newlines
        case '\n':
            addToken(TokenType::NEWLINE);
            line_++;
            break;

        default:
            if (isDigit(c)) {
                scanNumber();
            } else if (isAlpha(c)) {
                scanIdentifierOrKeyword();
            } else {
                std::cerr << "[line " << line_ << "] Lexer Error: Unexpected character '"
                          << c << "'" << std::endl;
                addToken(TokenType::UNKNOWN);
            }
            break;
    }
}

void Lexer::scanNumber() {
    while (!isAtEnd() && isDigit(peek())) advance();
    addToken(TokenType::NUMBER);
}

void Lexer::scanIdentifierOrKeyword() {
    while (!isAtEnd() && isAlphaNumeric(peek())) advance();
    std::string text = source_.substr(start_, current_ - start_);
    auto it = keywords_.find(text);
    addToken(it != keywords_.end() ? it->second : TokenType::IDENTIFIER);
}

void Lexer::scanString() {
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') line_++;
        advance();
    }

    if (isAtEnd()) {
        std::cerr << "[line " << line_ << "] Lexer Error: Unterminated string." << std::endl;
        addToken(TokenType::UNKNOWN);
        return;
    }

    advance(); // consume closing '"'

    // Extract contents without quotes
    std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
    tokens_.emplace_back(TokenType::STRING, std::move(value), line_);
}

// ---- Character helpers ----
char Lexer::advance()             { return source_[current_++]; }
char Lexer::peek() const          { return isAtEnd() ? '\0' : source_[current_]; }
char Lexer::peekNext() const      { return (current_ + 1 >= source_.size()) ? '\0' : source_[current_ + 1]; }
bool Lexer::isAtEnd() const       { return current_ >= source_.size(); }

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    current_++;
    return true;
}

bool Lexer::isDigit(char c)         { return c >= '0' && c <= '9'; }
bool Lexer::isAlpha(char c)         { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool Lexer::isAlphaNumeric(char c)  { return isAlpha(c) || isDigit(c); }

void Lexer::addToken(TokenType type) {
    std::string lexeme = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, std::move(lexeme), line_);
}

} // namespace cvm
