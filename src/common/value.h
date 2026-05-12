// ============================================================================
// CVM++ : common/value.h — Runtime Value Type
// ============================================================================
// The Value struct is the universal runtime representation used by the
// constant pool, operand stack, and global/local variable storage.
//
// DESIGN: Tagged-struct approach. Each Value carries a ValueType tag and
// the relevant payload field. This avoids std::variant (unavailable on
// GCC 6.3) while remaining type-safe and extensible.
//
// FUNCTION VALUES: Functions are first-class values. A FunctionObj holds
// the compiled bytecode (its own Chunk), parameter count, and name.
// We use std::shared_ptr for ownership because function values can be
// stored in the constant pool, on the stack, and in variables simultaneously.
// ============================================================================

#ifndef CVM_VALUE_H
#define CVM_VALUE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

namespace cvm {

// Forward declaration — full definition below (after Chunk)
struct FunctionObj;

// ============================================================================
// ValueType — every kind of runtime value
// ============================================================================

enum class ValueType {
    INTEGER,
    BOOLEAN,
    STRING,
    FUNCTION
};

// ============================================================================
// Value — a single runtime value
// ============================================================================

struct Value {
    ValueType   type;
    int         intVal  = 0;
    bool        boolVal = false;
    std::string strVal;
    std::shared_ptr<FunctionObj> funcVal;

    // Default: integer 0
    Value() : type(ValueType::INTEGER), intVal(0) {}

    // Convenience constructors
    explicit Value(int v)              : type(ValueType::INTEGER), intVal(v) {}
    explicit Value(bool v)             : type(ValueType::BOOLEAN), boolVal(v) {}
    explicit Value(const std::string& v) : type(ValueType::STRING), strVal(v) {}
    explicit Value(std::shared_ptr<FunctionObj> f)
        : type(ValueType::FUNCTION), funcVal(std::move(f)) {}
};

// ============================================================================
// Truthiness: 0, false, empty string, null function are falsy
// ============================================================================

inline bool isTruthy(const Value& v) {
    switch (v.type) {
        case ValueType::INTEGER:  return v.intVal != 0;
        case ValueType::BOOLEAN:  return v.boolVal;
        case ValueType::STRING:   return !v.strVal.empty();
        case ValueType::FUNCTION: return v.funcVal != nullptr;
        default:                  return false;
    }
}

// ============================================================================
// OpCode — every instruction the VM can execute
// ============================================================================
// Defined here so that Chunk (below) and the VM share the same enum
// without circular includes.
// ============================================================================

enum class OpCode : uint8_t {
    // ---- Constants & Literals ----
    OP_CONSTANT,        // [OP_CONSTANT] [index]
    OP_TRUE,
    OP_FALSE,

    // ---- Arithmetic ----
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEGATE,

    // ---- Logical ----
    OP_NOT,
    OP_AND,             // Pop b, pop a, push (truthy(a) && truthy(b))
    OP_OR,              // Pop b, pop a, push (truthy(a) || truthy(b))

    // ---- Comparison ----
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,

    // ---- Global Variables ----
    OP_SET_GLOBAL,      // [OP_SET_GLOBAL] [slot]
    OP_GET_GLOBAL,      // [OP_GET_GLOBAL] [slot]

    // ---- Local Variables ----
    OP_SET_LOCAL,       // [OP_SET_LOCAL] [slot]
    OP_GET_LOCAL,       // [OP_GET_LOCAL] [slot]

    // ---- Functions ----
    OP_CALL,            // [OP_CALL] [argCount]
    OP_RETURN,

    // ---- I/O ----
    OP_PRINT,
    OP_INPUT,

    // ---- Control Flow ----
    OP_JUMP,            // [OP_JUMP] [hi] [lo]
    OP_JUMP_IF_FALSE,   // [OP_JUMP_IF_FALSE] [hi] [lo]

    // ---- Stack ----
    OP_POP,

    // ---- Program ----
    OP_HALT
};

// ============================================================================
// opcodeToString — human-readable opcode names for disassembly
// ============================================================================

inline std::string opcodeToString(OpCode op) {
    switch (op) {
        case OpCode::OP_CONSTANT:       return "OP_CONSTANT";
        case OpCode::OP_TRUE:           return "OP_TRUE";
        case OpCode::OP_FALSE:          return "OP_FALSE";
        case OpCode::OP_ADD:            return "OP_ADD";
        case OpCode::OP_SUB:            return "OP_SUB";
        case OpCode::OP_MUL:            return "OP_MUL";
        case OpCode::OP_DIV:            return "OP_DIV";
        case OpCode::OP_NEGATE:         return "OP_NEGATE";
        case OpCode::OP_NOT:            return "OP_NOT";
        case OpCode::OP_AND:            return "OP_AND";
        case OpCode::OP_OR:             return "OP_OR";
        case OpCode::OP_EQUAL:          return "OP_EQUAL";
        case OpCode::OP_NOT_EQUAL:      return "OP_NOT_EQUAL";
        case OpCode::OP_LESS:           return "OP_LESS";
        case OpCode::OP_LESS_EQUAL:     return "OP_LESS_EQUAL";
        case OpCode::OP_GREATER:        return "OP_GREATER";
        case OpCode::OP_GREATER_EQUAL:  return "OP_GREATER_EQUAL";
        case OpCode::OP_SET_GLOBAL:     return "OP_SET_GLOBAL";
        case OpCode::OP_GET_GLOBAL:     return "OP_GET_GLOBAL";
        case OpCode::OP_SET_LOCAL:      return "OP_SET_LOCAL";
        case OpCode::OP_GET_LOCAL:      return "OP_GET_LOCAL";
        case OpCode::OP_CALL:           return "OP_CALL";
        case OpCode::OP_RETURN:         return "OP_RETURN";
        case OpCode::OP_PRINT:          return "OP_PRINT";
        case OpCode::OP_INPUT:          return "OP_INPUT";
        case OpCode::OP_JUMP:           return "OP_JUMP";
        case OpCode::OP_JUMP_IF_FALSE:  return "OP_JUMP_IF_FALSE";
        case OpCode::OP_POP:            return "OP_POP";
        case OpCode::OP_HALT:           return "OP_HALT";
        default:                        return "OP_UNKNOWN";
    }
}

// ============================================================================
// Chunk — a compiled bytecode sequence + constant pool
// ============================================================================

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value>   constants;
    std::vector<int>     lines;

    void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    int addConstant(const Value& value) {
        constants.push_back(value);
        return static_cast<int>(constants.size() - 1);
    }

    size_t size() const { return code.size(); }

    // Disassemble the entire chunk
    void disassemble(const std::string& name) const;

    // Disassemble a single instruction, return next offset
    size_t disassembleInstruction(size_t offset) const;
};

// ============================================================================
// FunctionObj — a compiled function
// ============================================================================

struct FunctionObj {
    std::string name;
    int         arity = 0;   // number of parameters
    Chunk       chunk;       // the function's own bytecode
};

// ============================================================================
// valueToString — now that FunctionObj is complete
// ============================================================================

inline std::string valueToString(const Value& v) {
    switch (v.type) {
        case ValueType::INTEGER:  return std::to_string(v.intVal);
        case ValueType::BOOLEAN:  return v.boolVal ? "true" : "false";
        case ValueType::STRING:   return v.strVal;
        case ValueType::FUNCTION:
            if (v.funcVal) return "<fn " + v.funcVal->name + ">";
            return "<fn>";
        default: return "???";
    }
}

// ============================================================================
// Chunk disassembly implementation (needs valueToString)
// ============================================================================

inline void Chunk::disassemble(const std::string& name) const {
    std::cout << "=== " << name << " ===" << std::endl;
    std::cout << std::endl;
    size_t offset = 0;
    while (offset < code.size()) {
        offset = disassembleInstruction(offset);
    }
    std::cout << std::endl;
}

inline size_t Chunk::disassembleInstruction(size_t offset) const {
    std::cout << "  "
              << std::right << std::setfill('0') << std::setw(4) << offset
              << std::setfill(' ');

    if (offset > 0 && lines[offset] == lines[offset - 1]) {
        std::cout << "    |  ";
    } else {
        std::cout << "  " << std::right << std::setw(3) << lines[offset] << "  ";
    }

    OpCode instruction = static_cast<OpCode>(code[offset]);
    std::string opName = opcodeToString(instruction);
    while (opName.size() < 22) opName += ' ';
    std::cout << opName;

    switch (instruction) {
        case OpCode::OP_CONSTANT: {
            uint8_t index = code[offset + 1];
            std::cout << static_cast<int>(index) << " ("
                      << valueToString(constants[index]) << ")";
            std::cout << std::endl;
            return offset + 2;
        }
        case OpCode::OP_SET_GLOBAL:
        case OpCode::OP_GET_GLOBAL:
        case OpCode::OP_SET_LOCAL:
        case OpCode::OP_GET_LOCAL:
        case OpCode::OP_CALL: {
            uint8_t operand = code[offset + 1];
            std::cout << static_cast<int>(operand);
            std::cout << std::endl;
            return offset + 2;
        }
        case OpCode::OP_JUMP:
        case OpCode::OP_JUMP_IF_FALSE: {
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

} // namespace cvm

#endif // CVM_VALUE_H
