// Kasturisundari Chain — Agni Compiler Implementation

#include "kasturisundari/vm/agni_compiler.h"
#include <sstream>
#include <iostream>

namespace kasturisundari {
namespace vm {

std::vector<AgniInstruction> AgniCompiler::compile(const std::string& source_code) {
    std::vector<AgniInstruction> bytecode;
    std::istringstream iss(source_code);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) words.push_back(word);

    size_t i = 0;
    while (i < words.size()) {
        bool found = false;
        
        // Try to match longest possible multi-word sutra (max 4 words)
        for (size_t len = std::min<size_t>(4, words.size() - i); len > 0; --len) {
            std::string candidate = words[i];
            for (size_t j = 1; j < len; ++j) candidate += " " + words[i + j];
            
            AgniOpcode op = AgniLexicon::get_sutra(candidate);
            if (op == AgniOpcode::OP_UNKNOWN) op = AgniLexicon::get_dhatu(candidate);
            
            if (op != AgniOpcode::OP_UNKNOWN) {
                bytecode.push_back({op, ""});
                i += len;
                found = true;
                break;
            }
        }

        if (!found) {
            std::string token = words[i];
            bool is_argument = true;
            for (char c : token) {
                // Allow digits and basic punctuation for args
                if (!std::isalnum(c) && c != '.' && c != '-') { is_argument = false; break; }
            }
            if (is_argument) {
                bytecode.push_back({AgniOpcode::OP_UNKNOWN, token});
                i++;
            } else {
                std::cerr << "[Agni Compiler] Syntax Error: Unknown Token: " << token << "\n";
                return {};
            }
        }
    }

    // Always append an END opcode if not present
    if (bytecode.empty() || bytecode.back().op != AgniOpcode::OP_END) {
        bytecode.push_back({AgniOpcode::OP_END, ""});
    }

    return bytecode;
}

} // namespace vm
} // namespace kasturisundari
