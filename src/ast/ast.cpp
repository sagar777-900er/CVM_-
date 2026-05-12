// ============================================================================
// CVM++ : ast/ast.cpp — AST Pretty-Printer Implementation
// ============================================================================

#include "ast.h"
#include <string>

namespace cvm {

static std::string indent(int depth) {
    return std::string(depth * 2, ' ');
}

std::string exprToString(const Expr& expr, int depth) {
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

        case ExprType::CALL: {
            std::string result = indent(depth) + "Call(" + expr.name + ")\n";
            for (size_t i = 0; i < expr.arguments.size(); ++i) {
                result += exprToString(*expr.arguments[i], depth + 1);
                if (i + 1 < expr.arguments.size()) result += "\n";
            }
            return result;
        }

        default:
            return indent(depth) + "???";
    }
}

std::string stmtToString(const Stmt& stmt, int depth) {
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

        case StmtType::FUNCTION: {
            std::string result = indent(depth) + "Function(" + stmt.name + ")\n";
            result += indent(depth + 1) + "Params: [";
            for (size_t i = 0; i < stmt.params.size(); ++i) {
                if (i > 0) result += ", ";
                result += stmt.params[i];
            }
            result += "]\n";
            result += indent(depth + 1) + "Body:\n";
            for (const auto& s : stmt.body) {
                result += stmtToString(s, depth + 2) + "\n";
            }
            return result;
        }

        case StmtType::RETURN: {
            std::string result = indent(depth) + "Return\n";
            if (stmt.expr) {
                result += exprToString(*stmt.expr, depth + 1);
            }
            return result;
        }

        default:
            return indent(depth) + "???";
    }
}

std::string programToString(const Program& program) {
    std::string result = "Program\n";
    for (const auto& stmt : program.statements) {
        result += stmtToString(stmt, 1) + "\n";
    }
    return result;
}

} // namespace cvm
