// ============================================================================
// CVM++ : lexer.h
// ============================================================================
// Declaration of the Lexer class — the first stage of the compiler pipeline.
//
// The Lexer (also called a Scanner or Tokenizer) reads raw source code
// character by character and produces a vector of Tokens.
//
// HOW IT WORKS (high level)
// -------------------------
// 1. The Lexer stores the entire source code as a std::string.
// 2. It maintains a "cursor" (the `current` index) that advances through
//    the string one character at a time.
// 3. At each step, it identifies what kind of token starts at the cursor:
//    - A digit?        → scan a number
//    - A letter?       → scan an identifier, then check the keyword table
//    - An operator?    → emit the operator token (handling two-char ones)
//    - Whitespace?     → skip it (but track newlines)
//    - Unknown char?   → emit an UNKNOWN error token
// 4. Each identified token is appended to a vector.
// 5. When the end of the source is reached, an EOF_TOKEN is appended.
//
// MEMORY MODEL
// ------------
// The Lexer OWNS its source string (passed by value or moved in).
// The returned token vector is moved out to the caller.
// No raw pointers, no manual memory management.  Everything is RAII.
//
// KEY VOCABULARY
// --------------
// - "advance"  = consume the current character and move the cursor forward
// - "peek"     = look at the current character WITHOUT consuming it
// - "match"    = consume the current character ONLY IF it equals an expected value
// - "lexeme"   = the substring of source code that forms a token
// - "start"    = the index where the current token began
// - "current"  = the index of the next character to read
// ============================================================================

#ifndef CVM_LEXER_H
#define CVM_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>

#include "token.h"

namespace cvm {

class Lexer {
public:
    // -----------------------------------------------------------------------
    // Constructor
    // -----------------------------------------------------------------------
    // Takes the source code string.  We accept by value so the caller can
    // either copy or std::move into us.
    // -----------------------------------------------------------------------
    explicit Lexer(std::string source);

    // -----------------------------------------------------------------------
    // tokenize — the main entry point
    // -----------------------------------------------------------------------
    // Scans the entire source and returns a vector of Tokens.
    // The last token is always EOF_TOKEN.
    // This method can only be called once meaningfully (it consumes the source).
    // -----------------------------------------------------------------------
    std::vector<Token> tokenize();

private:
    // ========================== SCANNER HELPERS ============================

    // scanToken — identifies and emits one token starting at `current`
    void scanToken();

    // scanNumber — called when the current character is a digit.
    //              Consumes all consecutive digits and emits a NUMBER token.
    void scanNumber();

    // scanIdentifierOrKeyword — called when the current character is a letter
    //              or underscore.  Consumes the full identifier, then checks
    //              if it's a reserved keyword.
    void scanIdentifierOrKeyword();

    // scanString — called when the current character is a double-quote.
    //              Consumes everything until the closing quote, emits STRING.
    void scanString();

    // ======================== CHARACTER HELPERS =============================

    // advance — consume the current character and return it.
    //           Moves `current` forward by 1.
    char advance();

    // peek — return the current character WITHOUT consuming it.
    //        Returns '\0' if at the end of the source.
    char peek() const;

    // peekNext — return the character AFTER the current one, without consuming.
    //            Used for two-character tokens like "==".
    //            Returns '\0' if out of bounds.
    char peekNext() const;

    // match — if the current character equals `expected`, consume it and
    //         return true.  Otherwise, return false and don't consume.
    //         This is the key to handling two-character tokens.
    bool match(char expected);

    // isAtEnd — have we consumed every character in the source?
    bool isAtEnd() const;

    // ===================== CHARACTER CLASSIFICATION ========================

    // These are static because they don't depend on any instance state.
    // They're just pure functions that classify a single character.

    static bool isDigit(char c);
    static bool isAlpha(char c);        // letters and underscore
    static bool isAlphaNumeric(char c); // letters, underscore, or digit

    // ======================== TOKEN CREATION ================================

    // addToken — extract the lexeme from source[start..current) and
    //            append a new Token to the tokens vector.
    void addToken(TokenType type);

    // ============================ DATA =====================================

    std::string          source_;   // the full source code (owned)
    std::vector<Token>   tokens_;   // accumulated tokens
    size_t               start_;    // index where the current token starts
    size_t               current_;  // index of the next character to read
    int                  line_;     // current line number (1-based)

    // Keyword lookup table — maps reserved words to their TokenType.
    // This is static because the keyword set is the same for every Lexer.
    static const std::unordered_map<std::string, TokenType> keywords_;
};

} // namespace cvm

#endif // CVM_LEXER_H
