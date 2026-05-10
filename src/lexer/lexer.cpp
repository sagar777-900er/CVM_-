// ============================================================================
// CVM++ : lexer.cpp — Lexer implementation
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

// Constructor — move the source string in, initialize cursor at position 0
Lexer::Lexer(std::string source)
    : source_(std::move(source)), start_(0), current_(0), line_(1) {}

// tokenize — main entry point. Scans all tokens and returns them.
std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start_ = current_;
        scanToken();
    }
    tokens_.emplace_back(TokenType::EOF_TOKEN, "", line_);
    return std::move(tokens_);
}

// scanToken — identify and emit one token
void Lexer::scanToken() {
    char c = advance();

    switch (c) {
        // Single-character tokens
        case '+': addToken(TokenType::PLUS);    break;
        case '-': addToken(TokenType::MINUS);   break;
        case '*': addToken(TokenType::STAR);    break;
        case '/':
            // Handle single-line comments: // ...
            if (match('/')) {
                while (!isAtEnd() && peek() != '\n') advance();
            } else {
                addToken(TokenType::SLASH);
            }
            break;
        case '(': addToken(TokenType::LPAREN);  break;
        case ')': addToken(TokenType::RPAREN);  break;
        case '{': addToken(TokenType::LBRACE);  break;
        case '}': addToken(TokenType::RBRACE);  break;

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

        // Delimiters
        case ';': addToken(TokenType::SEMICOLON); break;
        case ',': addToken(TokenType::COMMA);     break;

        // Whitespace
        case ' ': case '\t': case '\r':
            break; // skip

        // Newlines — emitted as tokens (statement separators)
        case '\n':
            addToken(TokenType::NEWLINE);
            line_++;
            break;

        default:
            if (isDigit(c)) {
                scanNumber();
            } else if (isAlpha(c)) {
                scanIdentifierOrKeyword();
            } else if (c == '"') {
                scanString();
            } else {
                std::cerr << "[Lexer Error] Unexpected character '"
                          << c << "' on line " << line_ << std::endl;
                addToken(TokenType::UNKNOWN);
            }
            break;
    }
}

// scanNumber — consume consecutive digits, emit NUMBER
void Lexer::scanNumber() {
    while (!isAtEnd() && isDigit(peek())) advance();
    addToken(TokenType::NUMBER);
}

// scanIdentifierOrKeyword — consume identifier, check keyword table
void Lexer::scanIdentifierOrKeyword() {
    while (!isAtEnd() && isAlphaNumeric(peek())) advance();
    std::string text = source_.substr(start_, current_ - start_);
    auto it = keywords_.find(text);
    addToken(it != keywords_.end() ? it->second : TokenType::IDENTIFIER);
}

// scanString — consume characters between double quotes, emit STRING
void Lexer::scanString() {
    // start_ currently points to the opening '"' character.
    // We consume everything until the closing '"'.
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') line_++;  // strings can span multiple lines
        advance();
    }

    if (isAtEnd()) {
        std::cerr << "[Lexer Error] Unterminated string on line " << line_ << std::endl;
        addToken(TokenType::UNKNOWN);
        return;
    }

    advance(); // consume the closing '"'

    // Extract the string contents (without the surrounding quotes)
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

// ---- Character classification ----
bool Lexer::isDigit(char c)         { return c >= '0' && c <= '9'; }
bool Lexer::isAlpha(char c)         { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool Lexer::isAlphaNumeric(char c)  { return isAlpha(c) || isDigit(c); }

// addToken — extract lexeme from source and append token
void Lexer::addToken(TokenType type) {
    std::string lexeme = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, std::move(lexeme), line_);
}

} // namespace cvm
