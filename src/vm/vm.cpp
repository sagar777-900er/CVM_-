// ============================================================================
// CVM++ : vm/vm.cpp — Virtual Machine Implementation
// ============================================================================

#include "vm.h"
#include <iostream>

namespace cvm {

// ============================================================================
// Constructor
// ============================================================================

VM::VM()
    : sp_(0), frameCount_(0), hadError_(false), traceExecution_(false) {
    globals_.resize(256);
}

// ============================================================================
// interpret — run a compiled chunk (top-level script)
// ============================================================================

VMResult VM::interpret(const Chunk& chunk) {
    // We treat the top-level chunk as a function with no parameters
    auto script = std::make_shared<FunctionObj>();
    script->name = "<script>";
    script->arity = 0;
    script->chunk = chunk;

    // Reset VM state
    sp_ = 0;
    frameCount_ = 0;
    hadError_ = false;

    // Call the script function
    push(Value(script)); // function value sits at basePointer
    callFunction(script, 0);

    return run();
}

// ============================================================================
// callFunction — set up a new CallFrame
// ============================================================================

bool VM::callFunction(std::shared_ptr<FunctionObj> func, int argCount) {
    if (argCount != func->arity) {
        runtimeError("Expected " + std::to_string(func->arity) +
                     " arguments but got " + std::to_string(argCount) + ".");
        return false;
    }

    if (frameCount_ == FRAMES_MAX) {
        runtimeError("Stack overflow (max frames exceeded).");
        return false;
    }

    CallFrame& frame = frames_[frameCount_++];
    frame.chunk = &func->chunk;
    frame.ip = 0;
    // The base of this frame is where the function object was pushed,
    // which is argCount slots down from the top of the stack.
    frame.basePointer = sp_ - argCount - 1;

    return true;
}

// ============================================================================
// Trace Helper
// ============================================================================

void VM::printStack() const {
    std::cout << "          ";
    for (int i = 0; i < sp_; i++) {
        std::cout << "[ " << valueToString(stack_[i]) << " ]";
    }
    std::cout << std::endl;
}

// ============================================================================
// run — the main dispatch loop
// ============================================================================

VMResult VM::run() {
    CallFrame* frame = &frames_[frameCount_ - 1];

    for (;;) {
        if (traceExecution_) {
            printStack();
            frame->chunk->disassembleInstruction(frame->ip);
        }

        uint8_t instruction = readByte();

        switch (static_cast<OpCode>(instruction)) {

            // ---- CONSTANTS & LITERALS ----
            case OpCode::OP_CONSTANT: {
                const Value& constant = readConstant();
                push(constant);
                break;
            }

            case OpCode::OP_TRUE:
                push(Value(true));
                break;

            case OpCode::OP_FALSE:
                push(Value(false));
                break;

            // ---- ARITHMETIC ----
            case OpCode::OP_ADD: {
                Value b = pop();
                Value a = pop();

                if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
                    push(Value(a.strVal + b.strVal));
                } else if (a.type == ValueType::STRING && b.type == ValueType::INTEGER) {
                    push(Value(a.strVal + std::to_string(b.intVal)));
                } else if (a.type == ValueType::INTEGER && b.type == ValueType::STRING) {
                    push(Value(std::to_string(a.intVal) + b.strVal));
                } else {
                    push(Value(a.intVal + b.intVal));
                }
                break;
            }

            case OpCode::OP_SUB: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal - b.intVal));
                break;
            }

            case OpCode::OP_MUL: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal * b.intVal));
                break;
            }

            case OpCode::OP_DIV: {
                Value b = pop();
                Value a = pop();
                if (b.intVal == 0) {
                    runtimeError("Division by zero.");
                    return VMResult::RUNTIME_ERROR;
                }
                push(Value(a.intVal / b.intVal));
                break;
            }

            case OpCode::OP_NEGATE: {
                Value a = pop();
                push(Value(-a.intVal));
                break;
            }

            // ---- LOGICAL ----
            case OpCode::OP_NOT: {
                Value a = pop();
                push(Value(!isTruthy(a)));
                break;
            }

            case OpCode::OP_AND: {
                Value b = pop();
                Value a = pop();
                push(Value(isTruthy(a) && isTruthy(b)));
                break;
            }

            case OpCode::OP_OR: {
                Value b = pop();
                Value a = pop();
                push(Value(isTruthy(a) || isTruthy(b)));
                break;
            }

            // ---- COMPARISON ----
            case OpCode::OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                if (a.type == ValueType::INTEGER && b.type == ValueType::INTEGER)
                    push(Value(a.intVal == b.intVal));
                else if (a.type == ValueType::BOOLEAN && b.type == ValueType::BOOLEAN)
                    push(Value(a.boolVal == b.boolVal));
                else if (a.type == ValueType::STRING && b.type == ValueType::STRING)
                    push(Value(a.strVal == b.strVal));
                else
                    push(Value(false));
                break;
            }

            case OpCode::OP_NOT_EQUAL: {
                Value b = pop();
                Value a = pop();
                if (a.type == ValueType::INTEGER && b.type == ValueType::INTEGER)
                    push(Value(a.intVal != b.intVal));
                else if (a.type == ValueType::BOOLEAN && b.type == ValueType::BOOLEAN)
                    push(Value(a.boolVal != b.boolVal));
                else if (a.type == ValueType::STRING && b.type == ValueType::STRING)
                    push(Value(a.strVal != b.strVal));
                else
                    push(Value(true));
                break;
            }

            case OpCode::OP_LESS: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal < b.intVal));
                break;
            }

            case OpCode::OP_LESS_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal <= b.intVal));
                break;
            }

            case OpCode::OP_GREATER: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal > b.intVal));
                break;
            }

            case OpCode::OP_GREATER_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(a.intVal >= b.intVal));
                break;
            }

            // ---- GLOBAL VARIABLES ----
            case OpCode::OP_SET_GLOBAL: {
                uint8_t slot = readByte();
                globals_[slot] = peek(0);
                break;
            }

            case OpCode::OP_GET_GLOBAL: {
                uint8_t slot = readByte();
                push(globals_[slot]);
                break;
            }

            // ---- LOCAL VARIABLES ----
            case OpCode::OP_SET_LOCAL: {
                uint8_t slot = readByte();
                stack_[frame->basePointer + 1 + slot] = peek(0);
                break;
            }

            case OpCode::OP_GET_LOCAL: {
                uint8_t slot = readByte();
                push(stack_[frame->basePointer + 1 + slot]);
                break;
            }

            // ---- FUNCTIONS ----
            case OpCode::OP_CALL: {
                int argCount = readByte();
                Value callee = peek(argCount);

                if (callee.type != ValueType::FUNCTION || callee.funcVal == nullptr) {
                    runtimeError("Can only call functions.");
                    return VMResult::RUNTIME_ERROR;
                }

                if (!callFunction(callee.funcVal, argCount)) {
                    return VMResult::RUNTIME_ERROR;
                }

                frame = &frames_[frameCount_ - 1]; // update current frame
                break;
            }

            case OpCode::OP_RETURN: {
                Value result = pop(); // grab return value
                int oldBase = frame->basePointer;

                frameCount_--;
                if (frameCount_ == 0) {
                    // Top-level script finished
                    pop(); // pop the script function object
                    return VMResult::OK;
                }

                // Discard call frame and its locals/args
                sp_ = oldBase;
                push(result); // push result onto caller's stack

                frame = &frames_[frameCount_ - 1]; // restore caller frame
                break;
            }

            // ---- I/O ----
            case OpCode::OP_PRINT: {
                Value value = pop();
                std::cout << valueToString(value) << std::endl;
                break;
            }

            case OpCode::OP_INPUT: {
                std::cout << "? ";
                int input;
                std::cin >> input;
                push(Value(input));
                break;
            }

            // ---- CONTROL FLOW ----
            case OpCode::OP_JUMP: {
                uint16_t target = readShort();
                frame->ip = target;
                break;
            }

            case OpCode::OP_JUMP_IF_FALSE: {
                uint16_t target = readShort();
                Value condition = pop();
                if (!isTruthy(condition)) {
                    frame->ip = target;
                }
                break;
            }

            // ---- STACK ----
            case OpCode::OP_POP: {
                pop();
                break;
            }

            // ---- HALT ----
            case OpCode::OP_HALT: {
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
    if (sp_ >= STACK_MAX) {
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

const Value& VM::peek(int distance) const {
    return stack_[sp_ - 1 - distance];
}

// ============================================================================
// BYTECODE READING
// ============================================================================

uint8_t VM::readByte() {
    return frames_[frameCount_ - 1].chunk->code[frames_[frameCount_ - 1].ip++];
}

const Value& VM::readConstant() {
    uint8_t index = readByte();
    return frames_[frameCount_ - 1].chunk->constants[index];
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
    CallFrame* frame = &frames_[frameCount_ - 1];
    int line = frame->chunk->lines[frame->ip > 0 ? frame->ip - 1 : 0];
    std::cerr << "[Runtime Error] Line " << line << " in " << frame->chunk << ": " << message << std::endl;
}

} // namespace cvm
