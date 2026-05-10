// ============================================================================
// CVM++ : compiler.cpp — Bytecode Compiler Implementation
// ============================================================================
//
// This file compiles an AST into bytecode by recursively visiting each node
// and emitting the appropriate instructions.
//
// KEY PRINCIPLE: Every expression leaves exactly ONE value on the stack.
// Every statement has a net-zero stack effect (it may push and pop internally,
// but the stack depth is the same before and after).
// ============================================================================

#include "compiler.h"
#include <iostream>
#include <utility>

namespace cvm {

// ============================================================================
// Constructor
// ============================================================================

Compiler::Compiler()
    : currentLine_(1), hadError_(false), nextGlobalSlot_(0) {}

// ============================================================================
// compile — main entry point
// ============================================================================

Chunk Compiler::compile(const Program& program) {
    chunk_ = Chunk();  // fresh chunk
    globals_.clear();
    nextGlobalSlot_ = 0;

    for (const auto& stmt : program.statements) {
        compileStatement(stmt);
    }

    emitByte(OP_HALT);
    return std::move(chunk_);
}

// ============================================================================
// STATEMENT COMPILATION
// ============================================================================

void Compiler::compileStatement(const Stmt& stmt) {
    switch (stmt.type) {

        // ---- LET: let x = expr ----
        // Compile the initializer (pushes value), then store it in a global slot.
        case StmtType::LET: {
            compileExpression(*stmt.expr);              // push initializer value
            int slot = resolveGlobal(stmt.name);        // get/create slot for name
            emitBytes(OP_SET_GLOBAL, static_cast<uint8_t>(slot));  // store it
            break;
        }

        // ---- PRINT: print expr ----
        // Compile the expression (pushes value), then print and pop it.
        case StmtType::PRINT: {
            compileExpression(*stmt.expr);              // push value
            emitByte(OP_PRINT);                         // print & pop
            break;
        }

        // ---- EXPRESSION STATEMENT: expr ----
        // Compile the expression, then discard the result (net-zero stack effect).
        case StmtType::EXPRESSION: {
            compileExpression(*stmt.expr);              // push value
            emitByte(OP_POP);                           // discard
            break;
        }

        // ---- BLOCK: { stmts... } ----
        case StmtType::BLOCK: {
            for (const auto& s : stmt.body) {
                compileStatement(s);
            }
            break;
        }

        // ---- IF: if condition { then } else { else } ----
        //
        // Compiled bytecode layout:
        //
        //   [condition code]           ← pushes true/false
        //   OP_JUMP_IF_FALSE  ──────┐  ← if false, jump to else/end
        //   [then branch code]      │
        //   OP_JUMP  ──────────────┐│  ← skip else branch
        //   [else branch code]  ←──┘│  ← jumped to by JUMP_IF_FALSE
        //   [continue...]      ←────┘  ← jumped to by JUMP
        //
        case StmtType::IF: {
            compileExpression(*stmt.expr);              // compile condition

            int jumpToElse = emitJump(OP_JUMP_IF_FALSE);  // placeholder jump

            // Compile "then" branch
            for (const auto& s : stmt.body) {
                compileStatement(s);
            }

            if (!stmt.elseBody.empty()) {
                int jumpOverElse = emitJump(OP_JUMP);   // skip else branch
                patchJump(jumpToElse);                  // else starts here

                for (const auto& s : stmt.elseBody) {
                    compileStatement(s);
                }

                patchJump(jumpOverElse);                // continue after else
            } else {
                patchJump(jumpToElse);                  // no else: jump to here
            }
            break;
        }

        // ---- WHILE: while condition { body } ----
        //
        // Compiled bytecode layout:
        //
        //   loopStart:
        //   [condition code]           ← pushes true/false
        //   OP_JUMP_IF_FALSE  ──────┐  ← if false, exit loop
        //   [body code]             │
        //   OP_JUMP (loop back)     │  ← unconditional jump to loopStart
        //   [continue...]       ←───┘  ← exit point
        //
        case StmtType::WHILE: {
            int loopStart = static_cast<int>(chunk_.size());  // remember start

            compileExpression(*stmt.expr);              // compile condition
            int exitJump = emitJump(OP_JUMP_IF_FALSE);  // exit if false

            // Compile body
            for (const auto& s : stmt.body) {
                compileStatement(s);
            }

            emitLoop(loopStart);                        // jump back to condition
            patchJump(exitJump);                        // exit point is here
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
// Every expression leaves exactly ONE value on the stack.
// ============================================================================

void Compiler::compileExpression(const Expr& expr) {
    switch (expr.type) {

        // ---- NUMBER LITERAL: 42 ----
        case ExprType::NUMBER_LITERAL: {
            emitConstant(Value(expr.numberValue));
            break;
        }

        // ---- STRING LITERAL: "hello" ----
        case ExprType::STRING_LITERAL: {
            emitConstant(Value(expr.stringValue));
            break;
        }

        // ---- BOOL LITERAL: true / false ----
        case ExprType::BOOL_LITERAL: {
            emitByte(expr.boolValue ? OP_TRUE : OP_FALSE);
            break;
        }

        // ---- IDENTIFIER: x ----
        // Look up the variable's global slot and push its value.
        case ExprType::IDENTIFIER: {
            auto it = globals_.find(expr.name);
            if (it == globals_.end()) {
                // Variable not yet declared — might be used before declaration
                // We'll assign a slot optimistically (the VM will catch uninitialized access)
                int slot = resolveGlobal(expr.name);
                emitBytes(OP_GET_GLOBAL, static_cast<uint8_t>(slot));
            } else {
                emitBytes(OP_GET_GLOBAL, static_cast<uint8_t>(it->second));
            }
            break;
        }

        // ---- UNARY: -x, !flag ----
        case ExprType::UNARY: {
            compileExpression(*expr.operand);     // push operand

            if (expr.op == "-") {
                emitByte(OP_NEGATE);
            } else if (expr.op == "!") {
                emitByte(OP_NOT);
            } else {
                error("Unknown unary operator: " + expr.op);
            }
            break;
        }

        // ---- BINARY: a + b, x == y, etc. ----
        case ExprType::BINARY: {
            // Compile both operands (each pushes one value)
            compileExpression(*expr.left);       // push left
            compileExpression(*expr.right);      // push right

            // Emit the appropriate operator instruction
            if      (expr.op == "+")  emitByte(OP_ADD);
            else if (expr.op == "-")  emitByte(OP_SUB);
            else if (expr.op == "*")  emitByte(OP_MUL);
            else if (expr.op == "/")  emitByte(OP_DIV);
            else if (expr.op == "==") emitByte(OP_EQUAL);
            else if (expr.op == "!=") emitByte(OP_NOT_EQUAL);
            else if (expr.op == "<")  emitByte(OP_LESS);
            else if (expr.op == "<=") emitByte(OP_LESS_EQUAL);
            else if (expr.op == ">")  emitByte(OP_GREATER);
            else if (expr.op == ">=") emitByte(OP_GREATER_EQUAL);
            // Logical AND: both must be truthy → implemented in the VM as
            // "if both are truthy, push true; else push false"
            // For simplicity, we use: (a != 0) * (b != 0) — but since our
            // VM already handles truthiness, we add a dedicated AND/OR in
            // the VM that checks truthiness of both operands.
            // For now, we compile "and" as OP_MUL (truthy*truthy=truthy,
            // anything*0=0) and "or" as OP_ADD + truthiness coercion.
            // Actually the cleanest approach: treat and/or as operators that
            // the VM handles by checking truthiness of both stack values.
            else if (expr.op == "and") {
                // Both values are on the stack. The VM's OP_MUL on integers
                // works for booleans: 1*1=1, 1*0=0, 0*0=0.
                // But we want proper boolean behavior, so let's use OP_EQUAL
                // trickery. Actually, simplest: just multiply (truthy values).
                emitByte(OP_MUL);
            }
            else if (expr.op == "or") {
                // a OR b: if either is truthy. Using ADD: 1+0=1, 0+1=1, 1+1=2(truthy)
                emitByte(OP_ADD);
            }
            else {
                error("Unknown binary operator: " + expr.op);
            }
            break;
        }

        // ---- ASSIGN: x = expr ----
        case ExprType::ASSIGN: {
            compileExpression(*expr.right);       // push the value
            int slot = resolveGlobal(expr.name);
            emitBytes(OP_SET_GLOBAL, static_cast<uint8_t>(slot));
            // Assignment is an expression that also leaves the value on the stack
            emitBytes(OP_GET_GLOBAL, static_cast<uint8_t>(slot));
            break;
        }

        // ---- INPUT ----
        case ExprType::INPUT: {
            emitByte(OP_INPUT);
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
    chunk_.write(byte, currentLine_);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitConstant(const Value& value) {
    int index = chunk_.addConstant(value);
    if (index > 255) {
        error("Too many constants in one chunk (max 256).");
        return;
    }
    emitBytes(OP_CONSTANT, static_cast<uint8_t>(index));
}

// emitJump — emit a jump instruction with a 2-byte placeholder offset.
// Returns the position of the first offset byte so we can patch it later.
int Compiler::emitJump(uint8_t jumpInstruction) {
    emitByte(jumpInstruction);
    emitByte(0xFF);  // placeholder hi byte
    emitByte(0xFF);  // placeholder lo byte
    return static_cast<int>(chunk_.size() - 2);  // position of hi byte
}

// patchJump — fill in a previously emitted jump to target the CURRENT position.
void Compiler::patchJump(int offset) {
    // Calculate how far to jump: from just after the jump instruction to here
    int target = static_cast<int>(chunk_.size());

    if (target > 65535) {
        error("Jump target too far (max 65535 bytes).");
        return;
    }

    chunk_.code[offset]     = static_cast<uint8_t>((target >> 8) & 0xFF);
    chunk_.code[offset + 1] = static_cast<uint8_t>(target & 0xFF);
}

// emitLoop — emit a jump backwards to the given start position.
void Compiler::emitLoop(int loopStart) {
    emitByte(OP_JUMP);
    int target = loopStart;

    if (target > 65535) {
        error("Loop target too far.");
        return;
    }

    emitByte(static_cast<uint8_t>((target >> 8) & 0xFF));
    emitByte(static_cast<uint8_t>(target & 0xFF));
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
