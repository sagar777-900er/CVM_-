// ============================================================================
// CVM++ : ast.h — Abstract Syntax Tree Node Types
// ============================================================================
//
// The AST (Abstract Syntax Tree) is the output of the Parser.  It represents
// the HIERARCHICAL structure of the program — unlike the flat token stream
// from the Lexer.
//
// For example, the expression "5 + 2 * 3" becomes:
//
//       (+)
//      /    \           <-- (tree branches)
//     5    (*)
//         /    \        <-- (tree branches)
//        2     3
//
// The tree captures operator PRECEDENCE: multiplication binds tighter than
// addition, so it appears deeper in the tree.  The VM will evaluate leaves
// first (bottom-up), which naturally computes 2*3=6, then 5+6=11.
//
// WHY THIS FILE EXISTS
// --------------------
// The Parser produces AST nodes.  The Compiler reads AST nodes.
// This file is the "contract" between them.
//
// DESIGN DECISIONS
// ----------------
// We use a tagged-enum approach where each node has an explicit ExprType
// or StmtType tag, plus a union-like struct with optional fields.  This
// avoids virtual functions, RTTI, and dynamic_cast while remaining
// compatible with C++17 on GCC 6.3.  Tree nodes are heap-allocated via
// std::unique_ptr.
// ============================================================================

#ifndef CVM_AST_H
#define CVM_AST_H

#include <memory>
#include <string>
#include <vector>
#include <iostream>
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
    INPUT
};

// Forward declaration
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// ============================================================================
// Expr — a single expression node
// ============================================================================
// Uses a type tag + fields.  Only the fields relevant to the ExprType are
// meaningful.  This is simpler than a class hierarchy for a learning project.
//
// Fields by type:
//   NUMBER_LITERAL  → numberValue
//   STRING_LITERAL  → stringValue
//   BOOL_LITERAL    → boolValue
//   IDENTIFIER      → name
//   UNARY           → op, operand
//   BINARY          → op, left, right
//   ASSIGN          → name, right (the value being assigned)
//   INPUT           → (no fields)
// ============================================================================

struct Expr {
    ExprType type;

    // Leaf data
    int         numberValue = 0;
    std::string stringValue;
    bool        boolValue = false;
    std::string name;       // identifier name or assignment target
    std::string op;         // operator lexeme ("+", "-", "==", etc.)

    // Children
    ExprPtr operand;        // for unary
    ExprPtr left;           // for binary
    ExprPtr right;          // for binary, assign
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

// ============================================================================
// Statement Types
// ============================================================================

enum class StmtType {
    LET,
    PRINT,
    EXPRESSION,
    BLOCK,
    IF,
    WHILE
};

// Forward declaration
struct Stmt;

// ============================================================================
// Stmt — a single statement node
// ============================================================================
// Fields by type:
//   LET        → name, expr
//   PRINT      → expr
//   EXPRESSION → expr
//   BLOCK      → body
//   IF         → expr (condition), body (then), elseBody (else)
//   WHILE      → expr (condition), body
// ============================================================================

struct Stmt {
    StmtType type;

    std::string name;           // variable name (for LET)
    ExprPtr     expr;           // expression (for LET initializer, PRINT, EXPRESSION, IF/WHILE condition)
    std::vector<Stmt> body;     // body statements (for BLOCK, IF then-branch, WHILE)
    std::vector<Stmt> elseBody; // else branch (for IF only)
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

// ============================================================================
// Program — the top-level AST node
// ============================================================================

struct Program {
    std::vector<Stmt> statements;
};

// ============================================================================
// AST Pretty-Printer (for debugging)
// ============================================================================

inline std::string indent(int depth) {
    return std::string(depth * 2, ' ');
}

// Forward declarations
inline std::string exprToString(const Expr& expr, int depth = 0);
inline std::string stmtToString(const Stmt& stmt, int depth = 0);

inline std::string exprToString(const Expr& expr, int depth) {
    switch (expr.type) {
        case ExprType::NUMBER_LITERAL:
            return indent(depth) + "Number(" + std::to_string(expr.numberValue) + ")";

        case ExprType::STRING_LITERAL:
            return indent(depth) + "String(\"" + expr.stringValue + "\")";

        case ExprType::BOOL_LITERAL:
            return indent(depth) + "Bool(" + (expr.boolValue ? "true" : "false") + ")";

        case ExprType::IDENTIFIER:
            return indent(depth) + "Ident(" + expr.name + ")";

        case ExprType::UNARY: {
            std::string result = indent(depth) + "Unary(" + expr.op + ")\n";
            result += exprToString(*expr.operand, depth + 1);
            return result;
        }

        case ExprType::BINARY: {
            std::string result = indent(depth) + "Binary(" + expr.op + ")\n";
            result += exprToString(*expr.left, depth + 1) + "\n";
            result += exprToString(*expr.right, depth + 1);
            return result;
        }

        case ExprType::ASSIGN: {
            std::string result = indent(depth) + "Assign(" + expr.name + ")\n";
            result += exprToString(*expr.right, depth + 1);
            return result;
        }

        case ExprType::INPUT:
            return indent(depth) + "Input";

        default:
            return indent(depth) + "???";
    }
}

inline std::string stmtToString(const Stmt& stmt, int depth) {
    switch (stmt.type) {
        case StmtType::LET: {
            std::string result = indent(depth) + "Let(" + stmt.name + ")\n";
            result += exprToString(*stmt.expr, depth + 1);
            return result;
        }

        case StmtType::PRINT: {
            std::string result = indent(depth) + "Print\n";
            result += exprToString(*stmt.expr, depth + 1);
            return result;
        }

        case StmtType::EXPRESSION: {
            std::string result = indent(depth) + "ExprStmt\n";
            result += exprToString(*stmt.expr, depth + 1);
            return result;
        }

        case StmtType::BLOCK: {
            std::string result = indent(depth) + "Block\n";
            for (const auto& s : stmt.body) {
                result += stmtToString(s, depth + 1) + "\n";
            }
            return result;
        }

        case StmtType::IF: {
            std::string result = indent(depth) + "If\n";
            result += indent(depth + 1) + "Condition:\n";
            result += exprToString(*stmt.expr, depth + 2) + "\n";
            result += indent(depth + 1) + "Then:\n";
            for (const auto& s : stmt.body) {
                result += stmtToString(s, depth + 2) + "\n";
            }
            if (!stmt.elseBody.empty()) {
                result += indent(depth + 1) + "Else:\n";
                for (const auto& s : stmt.elseBody) {
                    result += stmtToString(s, depth + 2) + "\n";
                }
            }
            return result;
        }

        case StmtType::WHILE: {
            std::string result = indent(depth) + "While\n";
            result += indent(depth + 1) + "Condition:\n";
            result += exprToString(*stmt.expr, depth + 2) + "\n";
            result += indent(depth + 1) + "Body:\n";
            for (const auto& s : stmt.body) {
                result += stmtToString(s, depth + 2) + "\n";
            }
            return result;
        }

        default:
            return indent(depth) + "???";
    }
}

inline std::string programToString(const Program& program) {
    std::string result = "Program\n";
    for (const auto& stmt : program.statements) {
        result += stmtToString(stmt, 1) + "\n";
    }
    return result;
}

} // namespace cvm

#endif // CVM_AST_H
