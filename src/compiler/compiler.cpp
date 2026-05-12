// ============================================================================
// CVM++ : compiler/compiler.cpp — Bytecode Compiler Implementation
// ============================================================================
// KEY PRINCIPLE: Every expression leaves exactly ONE value on the stack.
// Every statement has a net-zero stack effect.
// ============================================================================

#include "compiler.h"
#include <iostream>

namespace cvm {

// ============================================================================
// Constructor
// ============================================================================

Compiler::Compiler()
    : current_(nullptr), currentLine_(1), hadError_(false), nextGlobalSlot_(0) {}

// ============================================================================
// compile — main entry point
// ============================================================================

Chunk Compiler::compile(const Program& program, bool resetGlobals) {
    // Reset state
    topLevel_ = FunctionCompiler();
    current_ = &topLevel_;
    hadError_ = false;

    if (resetGlobals) {
        globals_.clear();
        nextGlobalSlot_ = 0;
    }

    for (const auto& stmt : program.statements) {
        compileStatement(stmt);
    }

    emitByte(static_cast<uint8_t>(OpCode::OP_HALT));
    return std::move(current_->function->chunk);
}

// ============================================================================
// currentChunk — get the chunk we're currently compiling into
// ============================================================================

Chunk& Compiler::currentChunk() {
    return current_->function->chunk;
}

// ============================================================================
// STATEMENT COMPILATION
// ============================================================================

void Compiler::compileStatement(const Stmt& stmt) {
    switch (stmt.type) {

        // ---- LET ----
        case StmtType::LET: {
            compileExpression(*stmt.expr);

            if (current_->scopeDepth > 0) {
                // Local variable: value is already on the stack at the right slot
                addLocal(stmt.name);
            } else {
                // Global variable
                int slot = resolveGlobal(stmt.name);
                emitBytes(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL),
                          static_cast<uint8_t>(slot));
            }
            break;
        }

        // ---- PRINT ----
        case StmtType::PRINT: {
            compileExpression(*stmt.expr);
            emitByte(static_cast<uint8_t>(OpCode::OP_PRINT));
            break;
        }

        // ---- EXPRESSION STATEMENT ----
        case StmtType::EXPRESSION: {
            compileExpression(*stmt.expr);
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            break;
        }

        // ---- BLOCK ----
        case StmtType::BLOCK: {
            beginScope();
            for (const auto& s : stmt.body) {
                compileStatement(s);
            }
            endScope();
            break;
        }

        // ---- IF ----
        case StmtType::IF: {
            compileExpression(*stmt.expr);
            int jumpToElse = emitJump(OpCode::OP_JUMP_IF_FALSE);

            for (const auto& s : stmt.body) {
                compileStatement(s);
            }

            if (!stmt.elseBody.empty()) {
                int jumpOverElse = emitJump(OpCode::OP_JUMP);
                patchJump(jumpToElse);
                for (const auto& s : stmt.elseBody) {
                    compileStatement(s);
                }
                patchJump(jumpOverElse);
            } else {
                patchJump(jumpToElse);
            }
            break;
        }

        // ---- WHILE ----
        case StmtType::WHILE: {
            int loopStart = static_cast<int>(currentChunk().size());
            compileExpression(*stmt.expr);
            int exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);

            for (const auto& s : stmt.body) {
                compileStatement(s);
            }

            emitLoop(loopStart);
            patchJump(exitJump);
            break;
        }

        // ---- FUNCTION DECLARATION ----
        // Compiles the function body into its own Chunk, then stores
        // the FunctionObj as a constant in the enclosing chunk.
        case StmtType::FUNCTION: {
            // Save current compiler state
            FunctionCompiler* enclosing = current_;

            // Create new function compiler
            FunctionCompiler funcCompiler;
            funcCompiler.function->name = stmt.name;
            funcCompiler.function->arity = static_cast<int>(stmt.params.size());
            funcCompiler.scopeDepth = 1; // function body is scope depth 1

            current_ = &funcCompiler;

            // Add parameters as locals
            for (const auto& param : stmt.params) {
                addLocal(param);
            }

            // Compile function body
            for (const auto& s : stmt.body) {
                compileStatement(s);
            }

            // Implicit return (push 0 and return)
            emitConstant(Value(0));
            emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));

            // Get the completed function
            std::shared_ptr<FunctionObj> func = funcCompiler.function;

            // Restore previous compiler state
            current_ = enclosing;

            // Store function as a constant and assign to global
            emitConstant(Value(func));
            int slot = resolveGlobal(stmt.name);
            emitBytes(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL),
                      static_cast<uint8_t>(slot));
            break;
        }

        // ---- RETURN ----
        case StmtType::RETURN: {
            if (stmt.expr) {
                compileExpression(*stmt.expr);
            } else {
                emitConstant(Value(0)); // return 0 if no expression
            }
            emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
            break;
        }

        default:
            error("Unknown statement type.");
            break;
    }
}

// ============================================================================
// EXPRESSION COMPILATION
// ============================================================================

void Compiler::compileExpression(const Expr& expr) {
    switch (expr.type) {

        case ExprType::NUMBER_LITERAL:
            emitConstant(Value(expr.numberValue));
            break;

        case ExprType::STRING_LITERAL:
            emitConstant(Value(expr.stringValue));
            break;

        case ExprType::BOOL_LITERAL:
            emitByte(static_cast<uint8_t>(
                expr.boolValue ? OpCode::OP_TRUE : OpCode::OP_FALSE));
            break;

        case ExprType::IDENTIFIER: {
            // First check locals, then globals
            int localSlot = resolveLocal(expr.name);
            if (localSlot != -1) {
                emitBytes(static_cast<uint8_t>(OpCode::OP_GET_LOCAL),
                          static_cast<uint8_t>(localSlot));
            } else {
                int globalSlot = resolveGlobal(expr.name);
                emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL),
                          static_cast<uint8_t>(globalSlot));
            }
            break;
        }

        case ExprType::UNARY: {
            compileExpression(*expr.operand);
            if (expr.op == "-") {
                emitByte(static_cast<uint8_t>(OpCode::OP_NEGATE));
            } else if (expr.op == "!") {
                emitByte(static_cast<uint8_t>(OpCode::OP_NOT));
            } else {
                error("Unknown unary operator: " + expr.op);
            }
            break;
        }

        case ExprType::BINARY: {
            compileExpression(*expr.left);
            compileExpression(*expr.right);

            if      (expr.op == "+")  emitByte(static_cast<uint8_t>(OpCode::OP_ADD));
            else if (expr.op == "-")  emitByte(static_cast<uint8_t>(OpCode::OP_SUB));
            else if (expr.op == "*")  emitByte(static_cast<uint8_t>(OpCode::OP_MUL));
            else if (expr.op == "/")  emitByte(static_cast<uint8_t>(OpCode::OP_DIV));
            else if (expr.op == "==") emitByte(static_cast<uint8_t>(OpCode::OP_EQUAL));
            else if (expr.op == "!=") emitByte(static_cast<uint8_t>(OpCode::OP_NOT_EQUAL));
            else if (expr.op == "<")  emitByte(static_cast<uint8_t>(OpCode::OP_LESS));
            else if (expr.op == "<=") emitByte(static_cast<uint8_t>(OpCode::OP_LESS_EQUAL));
            else if (expr.op == ">")  emitByte(static_cast<uint8_t>(OpCode::OP_GREATER));
            else if (expr.op == ">=") emitByte(static_cast<uint8_t>(OpCode::OP_GREATER_EQUAL));
            else if (expr.op == "and") emitByte(static_cast<uint8_t>(OpCode::OP_AND));
            else if (expr.op == "or")  emitByte(static_cast<uint8_t>(OpCode::OP_OR));
            else {
                error("Unknown binary operator: " + expr.op);
            }
            break;
        }

        case ExprType::ASSIGN: {
            compileExpression(*expr.right);

            // Check locals first
            int localSlot = resolveLocal(expr.name);
            if (localSlot != -1) {
                emitBytes(static_cast<uint8_t>(OpCode::OP_SET_LOCAL),
                          static_cast<uint8_t>(localSlot));
                // Assignment leaves value on stack
                emitBytes(static_cast<uint8_t>(OpCode::OP_GET_LOCAL),
                          static_cast<uint8_t>(localSlot));
            } else {
                int globalSlot = resolveGlobal(expr.name);
                emitBytes(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL),
                          static_cast<uint8_t>(globalSlot));
                emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL),
                          static_cast<uint8_t>(globalSlot));
            }
            break;
        }

        case ExprType::INPUT:
            emitByte(static_cast<uint8_t>(OpCode::OP_INPUT));
            break;

        // ---- FUNCTION CALL ----
        case ExprType::CALL: {
            // Push the function onto the stack
            int localSlot = resolveLocal(expr.name);
            if (localSlot != -1) {
                emitBytes(static_cast<uint8_t>(OpCode::OP_GET_LOCAL),
                          static_cast<uint8_t>(localSlot));
            } else {
                int globalSlot = resolveGlobal(expr.name);
                emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL),
                          static_cast<uint8_t>(globalSlot));
            }

            // Push arguments
            for (const auto& arg : expr.arguments) {
                compileExpression(*arg);
            }

            // Emit call with argument count
            emitBytes(static_cast<uint8_t>(OpCode::OP_CALL),
                      static_cast<uint8_t>(expr.arguments.size()));
            break;
        }

        default:
            error("Unknown expression type.");
            break;
    }
}

// ============================================================================
// EMIT HELPERS
// ============================================================================

void Compiler::emitByte(uint8_t byte) {
    currentChunk().write(byte, currentLine_);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitConstant(const Value& value) {
    int index = currentChunk().addConstant(value);
    if (index > 255) {
        error("Too many constants in one chunk (max 256).");
        return;
    }
    emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT),
              static_cast<uint8_t>(index));
}

int Compiler::emitJump(OpCode jumpInstruction) {
    emitByte(static_cast<uint8_t>(jumpInstruction));
    emitByte(0xFF);
    emitByte(0xFF);
    return static_cast<int>(currentChunk().size() - 2);
}

void Compiler::patchJump(int offset) {
    int target = static_cast<int>(currentChunk().size());
    if (target > 65535) {
        error("Jump target too far (max 65535 bytes).");
        return;
    }
    currentChunk().code[offset]     = static_cast<uint8_t>((target >> 8) & 0xFF);
    currentChunk().code[offset + 1] = static_cast<uint8_t>(target & 0xFF);
}

void Compiler::emitLoop(int loopStart) {
    emitByte(static_cast<uint8_t>(OpCode::OP_JUMP));
    if (loopStart > 65535) {
        error("Loop target too far.");
        return;
    }
    emitByte(static_cast<uint8_t>((loopStart >> 8) & 0xFF));
    emitByte(static_cast<uint8_t>(loopStart & 0xFF));
}

// ============================================================================
// SCOPE MANAGEMENT
// ============================================================================

void Compiler::beginScope() {
    current_->scopeDepth++;
}

void Compiler::endScope() {
    current_->scopeDepth--;

    // Pop all locals that belong to the scope we're leaving
    while (!current_->locals.empty() &&
           current_->locals.back().depth > current_->scopeDepth) {
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        current_->locals.pop_back();
    }
}

void Compiler::addLocal(const std::string& name) {
    Local local;
    local.name = name;
    local.depth = current_->scopeDepth;
    current_->locals.push_back(local);
}

int Compiler::resolveLocal(const std::string& name) {
    // Walk locals backwards to find the innermost matching variable
    for (int i = static_cast<int>(current_->locals.size()) - 1; i >= 0; i--) {
        if (current_->locals[i].name == name) {
            return i;
        }
    }
    return -1; // not a local
}

// ============================================================================
// SYMBOL TABLE
// ============================================================================

int Compiler::resolveGlobal(const std::string& name) {
    auto it = globals_.find(name);
    if (it != globals_.end()) {
        return it->second;
    }
    int slot = nextGlobalSlot_++;
    globals_[name] = slot;
    if (slot > 255) {
        error("Too many global variables (max 256).");
    }
    return slot;
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

void Compiler::error(const std::string& message) {
    hadError_ = true;
    std::cerr << "[Compiler Error] " << message << std::endl;
}

} // namespace cvm
