// ============================================================================
// CVM++ : ast/ast.h — Abstract Syntax Tree Node Types
// ============================================================================
// The AST is the output of the Parser. It represents the HIERARCHICAL
// structure of the program — unlike the flat token stream from the Lexer.
// ============================================================================

#ifndef CVM_AST_H
#define CVM_AST_H

#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace cvm {

// ============================================================================
// Expression Types
// ============================================================================

enum class ExprType {
    NUMBER_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,
    IDENTIFIER,
    UNARY,
    BINARY,
    ASSIGN,
    INPUT,
    CALL            // function call: name(args...)
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// ============================================================================
// Expr — a single expression node
// ============================================================================

struct Expr {
    ExprType type;

    // Leaf data
    int         numberValue = 0;
    std::string stringValue;
    bool        boolValue = false;
    std::string name;
    std::string op;

    // Children
    ExprPtr              operand;    // unary
    ExprPtr              left;       // binary
    ExprPtr              right;      // binary, assign
    std::vector<ExprPtr> arguments;  // call
};

// ============================================================================
// Expression factory helpers
// ============================================================================

inline ExprPtr makeNumberLiteral(int value) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::NUMBER_LITERAL;
    e->numberValue = value;
    return e;
}

inline ExprPtr makeStringLiteral(const std::string& value) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::STRING_LITERAL;
    e->stringValue = value;
    return e;
}

inline ExprPtr makeBoolLiteral(bool value) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::BOOL_LITERAL;
    e->boolValue = value;
    return e;
}

inline ExprPtr makeIdentifier(const std::string& name) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::IDENTIFIER;
    e->name = name;
    return e;
}

inline ExprPtr makeUnary(const std::string& op, ExprPtr operand) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::UNARY;
    e->op = op;
    e->operand = std::move(operand);
    return e;
}

inline ExprPtr makeBinary(const std::string& op, ExprPtr left, ExprPtr right) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::BINARY;
    e->op = op;
    e->left = std::move(left);
    e->right = std::move(right);
    return e;
}

inline ExprPtr makeAssign(const std::string& name, ExprPtr value) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::ASSIGN;
    e->name = name;
    e->right = std::move(value);
    return e;
}

inline ExprPtr makeInput() {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::INPUT;
    return e;
}

inline ExprPtr makeCall(const std::string& name, std::vector<ExprPtr> args) {
    auto e = std::make_unique<Expr>();
    e->type = ExprType::CALL;
    e->name = name;
    e->arguments = std::move(args);
    return e;
}

// ============================================================================
// Statement Types
// ============================================================================

enum class StmtType {
    LET,
    PRINT,
    EXPRESSION,
    BLOCK,
    IF,
    WHILE,
    FUNCTION,       // fn name(params) { body }
    RETURN          // return expr
};

struct Stmt;

// ============================================================================
// Stmt — a single statement node
// ============================================================================

struct Stmt {
    StmtType type;

    std::string              name;       // variable/function name
    ExprPtr                  expr;       // expression
    std::vector<Stmt>        body;       // block body / then branch / while body / function body
    std::vector<Stmt>        elseBody;   // else branch
    std::vector<std::string> params;     // function parameters
};

// ============================================================================
// Statement factory helpers
// ============================================================================

inline Stmt makeLetStmt(const std::string& name, ExprPtr initializer) {
    Stmt s;
    s.type = StmtType::LET;
    s.name = name;
    s.expr = std::move(initializer);
    return s;
}

inline Stmt makePrintStmt(ExprPtr expression) {
    Stmt s;
    s.type = StmtType::PRINT;
    s.expr = std::move(expression);
    return s;
}

inline Stmt makeExpressionStmt(ExprPtr expression) {
    Stmt s;
    s.type = StmtType::EXPRESSION;
    s.expr = std::move(expression);
    return s;
}

inline Stmt makeBlockStmt(std::vector<Stmt> statements) {
    Stmt s;
    s.type = StmtType::BLOCK;
    s.body = std::move(statements);
    return s;
}

inline Stmt makeIfStmt(ExprPtr condition, std::vector<Stmt> thenBranch,
                       std::vector<Stmt> elseBranch) {
    Stmt s;
    s.type = StmtType::IF;
    s.expr = std::move(condition);
    s.body = std::move(thenBranch);
    s.elseBody = std::move(elseBranch);
    return s;
}

inline Stmt makeWhileStmt(ExprPtr condition, std::vector<Stmt> body) {
    Stmt s;
    s.type = StmtType::WHILE;
    s.expr = std::move(condition);
    s.body = std::move(body);
    return s;
}

inline Stmt makeFunctionStmt(const std::string& name,
                             std::vector<std::string> params,
                             std::vector<Stmt> body) {
    Stmt s;
    s.type = StmtType::FUNCTION;
    s.name = name;
    s.params = std::move(params);
    s.body = std::move(body);
    return s;
}

inline Stmt makeReturnStmt(ExprPtr expression) {
    Stmt s;
    s.type = StmtType::RETURN;
    s.expr = std::move(expression);
    return s;
}

// ============================================================================
// Program — the top-level AST node
// ============================================================================

struct Program {
    std::vector<Stmt> statements;
};

// ============================================================================
// AST Pretty-Printer — declarations (implemented in ast.cpp)
// ============================================================================

std::string exprToString(const Expr& expr, int depth = 0);
std::string stmtToString(const Stmt& stmt, int depth = 0);
std::string programToString(const Program& program);

} // namespace cvm

#endif // CVM_AST_H
