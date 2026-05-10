// ============================================================================
// CVM++ : vm.h — Virtual Machine Declaration
// ============================================================================
//
// The VM is the final stage of the pipeline — the execution engine.
// It takes a compiled Chunk (bytecode + constant pool) and RUNS it.
//
//   Source → [Lexer] → Tokens → [Parser] → AST → [Compiler] → Chunk → [VM] → Output
//                                                                        ↑ YOU ARE HERE
//
// ARCHITECTURE: Stack-Based VM
// ----------------------------
// The CVM++ VM is a **stack-based** virtual machine.  This means:
//   - Values are pushed onto and popped from an operand stack
//   - Operators pop their arguments and push their results
//   - There are NO registers (unlike x86, ARM, or LLVM IR)
//
// This is the same architecture used by:
//   - Java Virtual Machine (JVM)
//   - Python's CPython interpreter
//   - .NET's Common Language Runtime (CLR)
//   - Lua's VM
//
// WHY STACK-BASED?
// ----------------
// Stack-based VMs are simpler to implement than register-based VMs.
// The compiler doesn't need to do register allocation — it just pushes
// and pops.  The tradeoff is slightly more instructions (each operation
// needs explicit push/pop), but for a learning project this is ideal.
//
// THE DISPATCH LOOP
// -----------------
// The VM's core is a tight loop:
//   1. FETCH:   read the next bytecode instruction
//   2. DECODE:  switch on the opcode
//   3. EXECUTE: perform the operation (push, pop, arithmetic, jump, etc.)
//   4. REPEAT
//
// This is called the "fetch-decode-execute" cycle — the same fundamental
// pattern used by real CPUs!
// ============================================================================

#ifndef CVM_VM_H
#define CVM_VM_H

#include <array>
#include <vector>
#include <string>

#include "../compiler/chunk.h"

namespace cvm {

// ============================================================================
// VMResult — outcome of VM execution
// ============================================================================

enum class VMResult {
    OK,             // program completed successfully
    RUNTIME_ERROR,  // a runtime error occurred
    COMPILE_ERROR   // shouldn't happen if compiler checked, but just in case
};

// ============================================================================
// VM — the stack-based virtual machine
// ============================================================================

class VM {
public:
    VM();

    // interpret — run a compiled chunk
    VMResult interpret(const Chunk& chunk);

private:
    // ======================== EXECUTION ====================================

    // run — the main dispatch loop
    VMResult run();

    // ======================== STACK OPERATIONS ==============================

    // push — push a value onto the operand stack
    void push(const Value& value);

    // pop — pop and return the top value from the stack
    Value pop();

    // peek — look at the top value without popping
    const Value& peek() const;

    // ======================== BYTECODE READING ==============================

    // readByte — read and return the next byte, advancing the IP
    uint8_t readByte();

    // readConstant — read a constant index and return the constant
    const Value& readConstant();

    // readShort — read a 2-byte offset (for jumps)
    uint16_t readShort();

    // ======================== ERROR HANDLING ================================

    void runtimeError(const std::string& message);

    // ============================ DATA =====================================

    const Chunk*                  chunk_;     // the chunk being executed
    size_t                        ip_;        // instruction pointer (index into code)
    std::array<Value, 256>        stack_;     // the operand stack (fixed size)
    int                           sp_;        // stack pointer (next free slot)
    std::vector<Value>            globals_;   // global variable storage
    bool                          hadError_;
};

} // namespace cvm

#endif // CVM_VM_H
