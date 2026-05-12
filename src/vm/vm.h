// ============================================================================
// CVM++ : vm/vm.h — Virtual Machine Declaration
// ============================================================================
// Stack-based VM with call frame support for function calls.
//
// CALL FRAME ARCHITECTURE:
//   Each function call creates a CallFrame that tracks:
//   - Which chunk is being executed (the function's bytecode)
//   - The instruction pointer within that chunk
//   - The base pointer (where this frame's stack window starts)
//
//   Stack layout during a call to f(a, b):
//   ┌──────────────────┐
//   │ ...caller data...│
//   │ <fn f>           │ ← basePointer (slot 0 from f's perspective)
//   │ arg a            │ ← local 0
//   │ arg b            │ ← local 1
//   │ ...f's locals... │
//   └──────────────────┘
// ============================================================================

#ifndef CVM_VM_H
#define CVM_VM_H

#include <array>
#include <vector>
#include <string>

#include "../common/value.h"

namespace cvm {

// ============================================================================
// CallFrame — one frame on the call stack
// ============================================================================

struct CallFrame {
    const Chunk*  chunk;       // bytecode being executed
    size_t        ip;          // instruction pointer
    int           basePointer; // base of this frame's stack window
};

// ============================================================================
// VMResult
// ============================================================================

enum class VMResult {
    OK,
    RUNTIME_ERROR,
    COMPILE_ERROR
};

// ============================================================================
// VM
// ============================================================================

class VM {
public:
    VM();

    VMResult interpret(const Chunk& chunk);

    // Access globals for REPL persistence
    std::vector<Value>& getGlobals() { return globals_; }
    const std::vector<Value>& getGlobals() const { return globals_; }

    // Trace mode: print each instruction and stack state
    void setTraceExecution(bool enabled) { traceExecution_ = enabled; }

private:
    VMResult run();

    // Stack operations
    void push(const Value& value);
    Value pop();
    const Value& peek(int distance = 0) const;

    // Bytecode reading (from current frame)
    uint8_t readByte();
    const Value& readConstant();
    uint16_t readShort();

    // Call a function
    bool callFunction(std::shared_ptr<FunctionObj> func, int argCount);

    // Error reporting
    void runtimeError(const std::string& message);

    // Trace helper
    void printStack() const;

    // ============================ DATA =====================================
    static const int STACK_MAX = 1024;
    static const int FRAMES_MAX = 64;

    std::array<Value, STACK_MAX>      stack_;
    int                               sp_;

    std::array<CallFrame, FRAMES_MAX> frames_;
    int                               frameCount_;

    std::vector<Value>                globals_;
    bool                              hadError_;
    bool                              traceExecution_;
};

} // namespace cvm

#endif // CVM_VM_H
