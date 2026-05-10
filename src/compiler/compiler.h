// ============================================================================
// CVM++ : compiler.h — Bytecode Compiler Declaration
// ============================================================================
//
// The Compiler walks the AST produced by the Parser and emits bytecode
// instructions into a Chunk.  This is the third stage of the pipeline:
//
//   Source → [Lexer] → Tokens → [Parser] → AST → [Compiler] → Bytecode
//                                                     ↑ YOU ARE HERE
//
// HOW IT WORKS
// ------------
// The Compiler does a recursive traversal of the AST.  For each node:
//   - Expressions:  emit code that leaves ONE value on the stack
//   - Statements:   emit code that has a NET ZERO stack effect
//
// For example, compiling "print 5 + 2":
//   1. Visit PrintStmt
//   2.   Visit BinaryExpr(+)
//   3.     Visit NumberLiteral(5) → emit OP_CONSTANT 5   (stack: [5])
//   4.     Visit NumberLiteral(2) → emit OP_CONSTANT 2   (stack: [5, 2])
//   5.     emit OP_ADD                                   (stack: [7])
//   6.   emit OP_PRINT                                   (stack: [])
//
// SYMBOL TABLE
// ------------
// Variables are stored as "globals" — the compiler maintains a map from
// variable names to slot indices.  When it sees "let x = 10", it assigns
// x the next available slot index and emits OP_SET_GLOBAL.  When it sees
// "x" used in an expression, it emits OP_GET_GLOBAL with x's slot.
//
// CONTROL FLOW (BACKPATCHING)
// ---------------------------
// For if/while statements, the compiler emits jump instructions with
// placeholder offsets, then FILLS THEM IN later once it knows the target
// address.  This is called "backpatching" — a standard compiler technique.
// ============================================================================

#ifndef CVM_COMPILER_H
#define CVM_COMPILER_H

#include <string>
#include <unordered_map>

#include "../ast/ast.h"
#include "chunk.h"

namespace cvm {

class Compiler {
public:
    Compiler();

    // compile — main entry point. Takes a Program AST, returns compiled Chunk.
    Chunk compile(const Program& program);

    // hadError — check if any compilation errors occurred
    bool hadError() const { return hadError_; }

private:
    // ======================== AST VISITORS =================================

    void compileStatement(const Stmt& stmt);
    void compileExpression(const Expr& expr);

    // ======================== EMIT HELPERS =================================

    // emit a single byte
    void emitByte(uint8_t byte);

    // emit two bytes (opcode + operand)
    void emitBytes(uint8_t byte1, uint8_t byte2);

    // emit a constant: adds to constant pool and emits OP_CONSTANT + index
    void emitConstant(const Value& value);

    // emit a jump instruction with placeholder offset, return the offset
    // position so we can patch it later
    int emitJump(uint8_t jumpInstruction);

    // patch a previously emitted jump to target the current position
    void patchJump(int offset);

    // emit a loop: jump backwards to the given position
    void emitLoop(int loopStart);

    // ======================== SYMBOL TABLE ==================================

    // Resolve a variable name to its global slot index.
    // If the variable hasn't been seen yet, assign it a new slot.
    int resolveGlobal(const std::string& name);

    // ======================== ERROR HANDLING =================================

    void error(const std::string& message);

    // ============================ DATA ======================================

    Chunk                                       chunk_;      // the bytecode being built
    int                                         currentLine_; // for line info
    bool                                        hadError_;
    std::unordered_map<std::string, int>        globals_;    // name → slot index
    int                                         nextGlobalSlot_;
};

} // namespace cvm

#endif // CVM_COMPILER_H
