// ============================================================================
// CVM++ : repl/repl.h — Interactive REPL Declaration
// ============================================================================
// REPL persists global variables across lines.
// ============================================================================

#ifndef CVM_REPL_H
#define CVM_REPL_H

#include <string>

#include "../compiler/compiler.h"
#include "../vm/vm.h"

namespace cvm {

class REPL {
public:
    REPL() = default;

    void run();

private:
    void processLine(const std::string& line);

    // Persistent compiler and VM to keep global state across lines
    Compiler compiler_;
    VM       vm_;
};

} // namespace cvm

#endif // CVM_REPL_H
