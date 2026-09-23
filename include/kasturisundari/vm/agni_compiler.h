// Kasturisundari Chain — Agni Compiler (अग्नि सङ्कलक)
// Parses Devanagari smart contract code and compiles it to Agni Bytecode.

#pragma once

#include "kasturisundari/vm/agni_machine.h"
#include <string>
#include <vector>

namespace kasturisundari {
namespace vm {

class AgniCompiler {
public:
    /// Compiles a string of Devanagari text into Agni Bytecode.
    /// Uses Panini's Sutras and Dhatupatha to map to opcodes.
    /// Returns an empty vector on syntax error.
    static std::vector<AgniInstruction> compile(const std::string& source_code);
};

} // namespace vm
} // namespace kasturisundari
