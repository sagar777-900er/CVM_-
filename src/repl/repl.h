// ============================================================================
// CVM++ : repl.h — Interactive REPL Declaration
// ============================================================================
//
// REPL = Read-Eval-Print Loop.  This gives the user an interactive
// shell where they can type CVM++ expressions and statements one
// line at a time and see immediate results.
//
// The REPL persists global variables across lines, so you can:
//   cvm> let x = 10
//   cvm> let y = 20
//   cvm> print x + y
//   30
//
// This is similar to Python's interactive mode, Node.js REPL, or irb (Ruby).
// ============================================================================

#ifndef CVM_REPL_H
#define CVM_REPL_H

#include <string>
#include <vector>
#include <unordered_map>

#include "../compiler/chunk.h"

namespace cvm {

class REPL {
public:
    REPL();

    // run — start the interactive REPL loop
    void run();

private:
    // processLine — compile and execute a single line of input
    void processLine(const std::string& line);

    // Global variable state persisted across lines
    std::vector<Value>                    globals_;
    std::unordered_map<std::string, int>  globalNames_;
    int                                   nextSlot_;
};

} // namespace cvm

#endif // CVM_REPL_H
