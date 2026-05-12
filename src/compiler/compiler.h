// ============================================================================
// CVM++ : compiler/compiler.h — Bytecode Compiler Declaration
// ============================================================================
// The Compiler walks the AST and emits bytecode into Chunks.
//
// KEY CONCEPTS:
//   - Local variables: tracked by the compiler using a scope stack.
//     Locals live on the VM stack; their slot is an offset from the
//     current call frame's base pointer.
//   - Functions: each function compiles to its own Chunk (FunctionObj).
//     The compiler uses a stack of "FunctionCompiler" states to handle
//     nested function definitions.
//   - Globals: identified at compile-time by name→slot mapping, shared
//     across all compilation contexts.
// ============================================================================

#ifndef CVM_COMPILER_H
#define CVM_COMPILER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "../ast/ast.h"
#include "../common/value.h"

namespace cvm {

// ============================================================================
// Local — a local variable tracked by the compiler
// ============================================================================

struct Local {
    std::string name;
    int         depth;  // scope depth (0 = global-level, 1+ = local)
};

// ============================================================================
// FunctionCompiler — state for compiling one function (or the top-level script)
// ============================================================================

struct FunctionCompiler {
    std::shared_ptr<FunctionObj>  function;
    std::vector<Local>            locals;
    int                           scopeDepth;

    FunctionCompiler()
        : function(std::make_shared<FunctionObj>()), scopeDepth(0) {
        function->name = "<script>";
    }
};

// ============================================================================
// Compiler
// ============================================================================

class Compiler {
public:
    Compiler();

    // compile — main entry point. Returns compiled Chunk for the top-level.
    // resetGlobals: if false, preserves global name→slot mapping (for REPL)
    Chunk compile(const Program& program, bool resetGlobals = true);

    bool hadError() const { return hadError_; }

private:
    // ======================== AST VISITORS =================================
    void compileStatement(const Stmt& stmt);
    void compileExpression(const Expr& expr);

    // ======================== EMIT HELPERS =================================
    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    void emitConstant(const Value& value);
    int  emitJump(OpCode jumpInstruction);
    void patchJump(int offset);
    void emitLoop(int loopStart);

    // ======================== SCOPE MANAGEMENT ==============================
    void beginScope();
    void endScope();
    void addLocal(const std::string& name);
    int  resolveLocal(const std::string& name);

    // ======================== SYMBOL TABLE ==================================
    int resolveGlobal(const std::string& name);

    // ======================== FUNCTION COMPILATION ==========================
    Chunk& currentChunk();

    // ======================== ERROR HANDLING =================================
    void error(const std::string& message);

    // ============================ DATA ======================================
    FunctionCompiler*  current_;         // current function being compiled
    FunctionCompiler   topLevel_;        // the top-level script compiler

    int   currentLine_;
    bool  hadError_;

    // Global symbol table (shared across all functions)
    std::unordered_map<std::string, int>  globals_;
    int                                   nextGlobalSlot_;
};

} // namespace cvm

#endif // CVM_COMPILER_H
