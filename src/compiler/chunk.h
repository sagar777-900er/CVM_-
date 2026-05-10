// ============================================================================
// CVM++ : chunk.h — Bytecode Chunk
// ============================================================================
//
// A Chunk is a container for compiled bytecode.  It holds:
//   1. The bytecode stream (a vector of bytes)
//   2. The constant pool (integer and string values referenced by bytecode)
//   3. Line number information (for runtime error messages)
//
// WHY "CHUNK"?
// ------------
// The name comes from Lua's implementation.  A chunk is a "chunk of code" —
// the compiled form of a function or script.  In our case, the entire
// program compiles into a single chunk.
//
// CONSTANT POOL
// -------------
// When the compiler encounters a literal like 42, it doesn't embed "42"
// directly into the bytecode stream.  Instead:
//   1. It adds 42 to the constant pool (a vector of Values)
//   2. It emits OP_CONSTANT followed by the INDEX into that pool
//
// This is more efficient and matches how real VMs work (JVM, CPython).
// ============================================================================

#ifndef CVM_CHUNK_H
#define CVM_CHUNK_H

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

#include "opcode.h"

namespace cvm {

// ============================================================================
// Value — a runtime value in the VM
// ============================================================================
// For now, values are either integers, booleans, or strings.
// We use a tagged union approach (same as the AST).
// ============================================================================

enum class ValueType {
    INTEGER,
    BOOLEAN,
    STRING
};

struct Value {
    ValueType type;
    int       intVal  = 0;
    bool      boolVal = false;
    std::string strVal;

    // Convenience constructors
    Value() : type(ValueType::INTEGER), intVal(0) {}
    explicit Value(int v)  : type(ValueType::INTEGER), intVal(v) {}
    explicit Value(bool v) : type(ValueType::BOOLEAN), boolVal(v) {}
    explicit Value(const std::string& v) : type(ValueType::STRING), strVal(v) {}
};

// Print a Value for debugging / OP_PRINT
inline std::string valueToString(const Value& v) {
    switch (v.type) {
        case ValueType::INTEGER: return std::to_string(v.intVal);
        case ValueType::BOOLEAN: return v.boolVal ? "true" : "false";
        case ValueType::STRING:  return v.strVal;
        default:                 return "???";
    }
}

// Truthiness: 0 and false are falsy, everything else is truthy
inline bool isTruthy(const Value& v) {
    switch (v.type) {
        case ValueType::INTEGER: return v.intVal != 0;
        case ValueType::BOOLEAN: return v.boolVal;
        case ValueType::STRING:  return !v.strVal.empty();
        default:                 return false;
    }
}

// ============================================================================
// Chunk — a compiled bytecode sequence
// ============================================================================

struct Chunk {
    std::vector<uint8_t> code;       // the bytecode stream
    std::vector<Value>   constants;  // the constant pool
    std::vector<int>     lines;      // source line for each byte (for errors)

    // write — append a byte to the bytecode stream
    void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    // addConstant — add a value to the constant pool, return its index
    int addConstant(const Value& value) {
        constants.push_back(value);
        return static_cast<int>(constants.size() - 1);
    }

    // size — number of bytes in the bytecode stream
    size_t size() const { return code.size(); }

    // ====================================================================
    // disassemble — print human-readable bytecode listing
    // ====================================================================
    // Output format:
    //   Offset  Line  Instruction       Operand
    //   0000    1     OP_CONSTANT       0 (42)
    //   0002    1     OP_CONSTANT       1 (10)
    //   0004    1     OP_ADD
    //   0005    1     OP_PRINT
    // ====================================================================

    void disassemble(const std::string& name) const {
        std::cout << "=== " << name << " ===" << std::endl;
        std::cout << std::endl;

        size_t offset = 0;
        while (offset < code.size()) {
            offset = disassembleInstruction(offset);
        }
        std::cout << std::endl;
    }

    size_t disassembleInstruction(size_t offset) const {
        // Print offset (4-digit, zero-padded)
        std::cout << "  "
                  << std::right << std::setfill('0') << std::setw(4) << offset
                  << std::setfill(' ');

        // Print line number
        if (offset > 0 && lines[offset] == lines[offset - 1]) {
            std::cout << "    |  ";
        } else {
            std::cout << "  " << std::right << std::setw(3) << lines[offset] << "  ";
        }

        uint8_t instruction = code[offset];
        std::string opName = opcodeToString(instruction);

        // Pad opcode name to fixed width
        while (opName.size() < 22) opName += ' ';
        std::cout << opName;

        switch (instruction) {
            case OP_CONSTANT: {
                uint8_t index = code[offset + 1];
                std::cout << static_cast<int>(index) << " ("
                          << valueToString(constants[index]) << ")";
                std::cout << std::endl;
                return offset + 2;
            }

            case OP_SET_GLOBAL:
            case OP_GET_GLOBAL: {
                uint8_t index = code[offset + 1];
                std::cout << static_cast<int>(index);
                std::cout << std::endl;
                return offset + 2;
            }

            case OP_JUMP:
            case OP_JUMP_IF_FALSE: {
                uint8_t hi = code[offset + 1];
                uint8_t lo = code[offset + 2];
                int target = (hi << 8) | lo;
                std::cout << "-> " << target;
                std::cout << std::endl;
                return offset + 3;
            }

            default:
                std::cout << std::endl;
                return offset + 1;
        }
    }
};

} // namespace cvm

#endif // CVM_CHUNK_H
