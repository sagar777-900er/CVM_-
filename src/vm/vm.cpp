// ============================================================================
// CVM++ : vm.cpp — Virtual Machine Implementation
// ============================================================================
//
// This file implements the stack-based virtual machine.
//
// THE DISPATCH LOOP
// -----------------
// The heart of the VM is a single for(;;) loop with a switch statement.
// Each case handles one opcode.  This pattern is called a
// "switch-threaded" interpreter — the simplest and most common design.
//
// STACK VISUALIZATION
// -------------------
// For "let x = 5 + 2":
//
//   Instruction          Stack (top →)
//   ────────────         ──────────────
//   OP_CONSTANT 5        [5]
//   OP_CONSTANT 2        [5, 2]
//   OP_ADD                [7]
//   OP_SET_GLOBAL 0      []          ← stored 7 in globals[0]
//
// For "print x + 3":
//
//   Instruction          Stack (top →)
//   ────────────         ──────────────
//   OP_GET_GLOBAL 0      [7]         ← loaded from globals[0]
//   OP_CONSTANT 3        [7, 3]
//   OP_ADD                [10]
//   OP_PRINT              []          ← printed "10"
// ============================================================================

#include "vm.h"
#include <iostream>

namespace cvm {

// ============================================================================
// Constructor
// ============================================================================

VM::VM()
    : chunk_(nullptr), ip_(0), sp_(0), hadError_(false) {
    // Pre-allocate globals (256 slots max)
    globals_.resize(256);
}

// ============================================================================
// interpret — run a compiled chunk
// ============================================================================

VMResult VM::interpret(const Chunk& chunk) {
    chunk_ = &chunk;
    ip_ = 0;
    sp_ = 0;
    hadError_ = false;

    return run();
}

// ============================================================================
// run — the main dispatch loop (fetch-decode-execute)
// ============================================================================

VMResult VM::run() {
    for (;;) {
        uint8_t instruction = readByte();

        switch (instruction) {

            // ---- CONSTANTS & LITERALS ----

            case OP_CONSTANT: {
                const Value& constant = readConstant();
                push(constant);
                break;
            }

            case OP_TRUE: {
                push(Value(true));
                break;
            }

            case OP_FALSE: {
                push(Value(false));
                break;
            }

            // ---- ARITHMETIC ----

            case OP_ADD: {
                Value b = pop();
                Value a = pop();

                // String concatenation
                if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
                    push(Value(a.strVal + b.strVal));
                }
                // String + number → concatenation
                else if (a.type == ValueType::STRING && b.type == ValueType::INTEGER) {
                    push(Value(a.strVal + std::to_string(b.intVal)));
                }
                else if (a.type == ValueType::INTEGER && b.type == ValueType::STRING) {
                    push(Value(std::to_string(a.intVal) + b.strVal));
                }
                // Numeric addition
                else {
                    push(Value(a.intVal + b.intVal));
                }
                break;
            }

            case OP_SUB: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal - b.intVal));
                break;
            }

            case OP_MUL: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal * b.intVal));
                break;
            }

            case OP_DIV: {
                Value b = pop();
                Value a = pop();
                if (b.intVal == 0) {
                    runtimeError("Division by zero.");
                    return VMResult::RUNTIME_ERROR;
                }
                push(Value(a.intVal / b.intVal));
                break;
            }

            case OP_NEGATE: {
                Value a = pop();
                push(Value(-a.intVal));
                break;
            }

            // ---- LOGICAL ----

            case OP_NOT: {
                Value a = pop();
                push(Value(!isTruthy(a)));
                break;
            }

            // ---- COMPARISON ----

            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                if (a.type == ValueType::INTEGER && b.type == ValueType::INTEGER)
                    push(Value(a.intVal == b.intVal));
                else if (a.type == ValueType::BOOLEAN && b.type == ValueType::BOOLEAN)
                    push(Value(a.boolVal == b.boolVal));
                else if (a.type == ValueType::STRING && b.type == ValueType::STRING)
                    push(Value(a.strVal == b.strVal));
                else
                    push(Value(false));  // different types are not equal
                break;
            }

            case OP_NOT_EQUAL: {
                Value b = pop();
                Value a = pop();
                if (a.type == ValueType::INTEGER && b.type == ValueType::INTEGER)
                    push(Value(a.intVal != b.intVal));
                else if (a.type == ValueType::BOOLEAN && b.type == ValueType::BOOLEAN)
                    push(Value(a.boolVal != b.boolVal));
                else if (a.type == ValueType::STRING && b.type == ValueType::STRING)
                    push(Value(a.strVal != b.strVal));
                else
                    push(Value(true));  // different types are not equal
                break;
            }

            case OP_LESS: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal < b.intVal));
                break;
            }

            case OP_LESS_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal <= b.intVal));
                break;
            }

            case OP_GREATER: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal > b.intVal));
                break;
            }

            case OP_GREATER_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal >= b.intVal));
                break;
            }

            // ---- VARIABLES ----

            case OP_SET_GLOBAL: {
                uint8_t slot = readByte();
                globals_[slot] = pop();
                break;
            }

            case OP_GET_GLOBAL: {
                uint8_t slot = readByte();
                push(globals_[slot]);
                break;
            }

            // ---- I/O ----

            case OP_PRINT: {
                Value value = pop();
                std::cout << valueToString(value) << std::endl;
                break;
            }

            case OP_INPUT: {
                std::cout << "? ";
                int input;
                std::cin >> input;
                push(Value(input));
                break;
            }

            // ---- CONTROL FLOW ----

            case OP_JUMP: {
                uint16_t target = readShort();
                ip_ = target;
                break;
            }

            case OP_JUMP_IF_FALSE: {
                uint16_t target = readShort();
                Value condition = pop();
                if (!isTruthy(condition)) {
                    ip_ = target;
                }
                break;
            }

            // ---- STACK ----

            case OP_POP: {
                pop();
                break;
            }

            // ---- HALT ----

            case OP_HALT: {
                return VMResult::OK;
            }

            default: {
                runtimeError("Unknown opcode: " + std::to_string(instruction));
                return VMResult::RUNTIME_ERROR;
            }
        }
    }
}

// ============================================================================
// STACK OPERATIONS
// ============================================================================

void VM::push(const Value& value) {
    if (sp_ >= 256) {
        runtimeError("Stack overflow.");
        return;
    }
    stack_[sp_++] = value;
}

Value VM::pop() {
    if (sp_ <= 0) {
        runtimeError("Stack underflow.");
        return Value(0);
    }
    return stack_[--sp_];
}

const Value& VM::peek() const {
    return stack_[sp_ - 1];
}

// ============================================================================
// BYTECODE READING
// ============================================================================

uint8_t VM::readByte() {
    return chunk_->code[ip_++];
}

const Value& VM::readConstant() {
    uint8_t index = readByte();
    return chunk_->constants[index];
}

uint16_t VM::readShort() {
    uint8_t hi = readByte();
    uint8_t lo = readByte();
    return static_cast<uint16_t>((hi << 8) | lo);
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

void VM::runtimeError(const std::string& message) {
    hadError_ = true;
    int line = chunk_->lines[ip_ > 0 ? ip_ - 1 : 0];
    std::cerr << "[Runtime Error] Line " << line << ": " << message << std::endl;
}

} // namespace cvm
