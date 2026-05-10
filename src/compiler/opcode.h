// ============================================================================
// CVM++ : opcode.h — Instruction Set Definition
// ============================================================================
//
// This file defines every bytecode instruction that the CVM++ virtual
// machine understands.  Each instruction is a single byte (uint8_t).
// Some instructions are followed by an operand byte (e.g. the index
// into the constant pool).
//
// WHY BYTECODE?
// -------------
// Walking an AST tree for every execution is slow — you chase pointers,
// branch on node types, and do lots of redundant work.  Bytecode is a
// flat array of bytes that the VM can blast through with a simple loop.
// This is how Python (CPython), Lua, Java (JVM), and Ruby work.
//
// INSTRUCTION FORMAT
// ------------------
//   [opcode]                — 1-byte instructions (no operand)
//   [opcode] [operand]      — 2-byte instructions (1-byte operand)
//   [opcode] [hi] [lo]      — 3-byte instructions (2-byte operand, for jumps)
//
// The operand for OP_CONSTANT, OP_SET_GLOBAL, OP_GET_GLOBAL is a 1-byte
// index (0–255) into a table.  For jumps, we use 2 bytes (0–65535).
// ============================================================================

#ifndef CVM_OPCODE_H
#define CVM_OPCODE_H

#include <cstdint>
#include <string>

namespace cvm {

// ============================================================================
// OpCode — every instruction the VM can execute
// ============================================================================

enum OpCode : uint8_t {
    // ---- Constants & Literals ----
    OP_CONSTANT,        // Push constant from pool:  [OP_CONSTANT] [index]
    OP_TRUE,            // Push boolean true
    OP_FALSE,           // Push boolean false

    // ---- Arithmetic ----
    OP_ADD,             // Pop b, pop a, push a + b
    OP_SUB,             // Pop b, pop a, push a - b
    OP_MUL,             // Pop b, pop a, push a * b
    OP_DIV,             // Pop b, pop a, push a / b
    OP_NEGATE,          // Pop a, push -a

    // ---- Logical ----
    OP_NOT,             // Pop a, push !a

    // ---- Comparison ----
    OP_EQUAL,           // Pop b, pop a, push (a == b)
    OP_NOT_EQUAL,       // Pop b, pop a, push (a != b)
    OP_LESS,            // Pop b, pop a, push (a < b)
    OP_LESS_EQUAL,      // Pop b, pop a, push (a <= b)
    OP_GREATER,         // Pop b, pop a, push (a > b)
    OP_GREATER_EQUAL,   // Pop b, pop a, push (a >= b)

    // ---- Variables ----
    OP_SET_GLOBAL,      // Pop value, store in globals[index]:  [OP_SET_GLOBAL] [index]
    OP_GET_GLOBAL,      // Push globals[index]:                 [OP_GET_GLOBAL] [index]

    // ---- I/O ----
    OP_PRINT,           // Pop value and print it
    OP_INPUT,           // Read integer from stdin, push it

    // ---- Control Flow ----
    OP_JUMP,            // Unconditional jump:  [OP_JUMP] [hi] [lo]
    OP_JUMP_IF_FALSE,   // Pop; jump if falsy:  [OP_JUMP_IF_FALSE] [hi] [lo]

    // ---- Stack Management ----
    OP_POP,             // Discard top of stack

    // ---- Program Control ----
    OP_HALT             // Stop execution
};

// ============================================================================
// opcodeToString — human-readable names for disassembly
// ============================================================================

inline std::string opcodeToString(uint8_t op) {
    switch (op) {
        case OP_CONSTANT:       return "OP_CONSTANT";
        case OP_TRUE:           return "OP_TRUE";
        case OP_FALSE:          return "OP_FALSE";
        case OP_ADD:            return "OP_ADD";
        case OP_SUB:            return "OP_SUB";
        case OP_MUL:            return "OP_MUL";
        case OP_DIV:            return "OP_DIV";
        case OP_NEGATE:         return "OP_NEGATE";
        case OP_NOT:            return "OP_NOT";
        case OP_EQUAL:          return "OP_EQUAL";
        case OP_NOT_EQUAL:      return "OP_NOT_EQUAL";
        case OP_LESS:           return "OP_LESS";
        case OP_LESS_EQUAL:     return "OP_LESS_EQUAL";
        case OP_GREATER:        return "OP_GREATER";
        case OP_GREATER_EQUAL:  return "OP_GREATER_EQUAL";
        case OP_SET_GLOBAL:     return "OP_SET_GLOBAL";
        case OP_GET_GLOBAL:     return "OP_GET_GLOBAL";
        case OP_PRINT:          return "OP_PRINT";
        case OP_INPUT:          return "OP_INPUT";
        case OP_JUMP:           return "OP_JUMP";
        case OP_JUMP_IF_FALSE:  return "OP_JUMP_IF_FALSE";
        case OP_POP:            return "OP_POP";
        case OP_HALT:           return "OP_HALT";
        default:                return "OP_UNKNOWN";
    }
}

} // namespace cvm

#endif // CVM_OPCODE_H
