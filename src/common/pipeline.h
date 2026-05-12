// ============================================================================
// CVM++ : common/pipeline.h — Pipeline Driver
// ============================================================================
// Helper functions that orchestrate the pipeline: Lexer -> Parser -> Compiler -> VM
// This is used by main.cpp (CLI), tests, and the REPL.
// ============================================================================

#ifndef CVM_PIPELINE_H
#define CVM_PIPELINE_H

#include <string>
#include "../vm/vm.h"

namespace cvm {

void tokenizeAndPrint(const std::string& label, const std::string& source);
void parseAndPrint(const std::string& label, const std::string& source);
void compileAndPrint(const std::string& label, const std::string& source);
void runProgram(const std::string& label, const std::string& source, bool showOutput = true, bool trace = false);

// Run test suite
void runTests();

} // namespace cvm

#endif // CVM_PIPELINE_H
