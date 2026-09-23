// Kasturisundari Chain — Agni Virtual Machine (अग्नि यन्त्र)
// Executes Agni Bytecode (KAB) for on-chain smart contracts.

#pragma once

#include "kasturisundari/vm/agni_lexicon.h"
#include "kasturisundari/state/state_db.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace kasturisundari {
namespace vm {

/// Represents the persistent storage state of a smart contract
using ContractState = std::unordered_map<std::string, std::string>;

/// Represents a single instruction with an optional argument
struct AgniInstruction {
    AgniOpcode op;
    std::string arg;
};

class AgniMachine {
public:
    AgniMachine(uint64_t initial_prana = 10000); // Gas limit

    /// Loads Agni Bytecode into the VM
    void load_bytecode(const std::vector<AgniInstruction>& bytecode);

    /// Executes the loaded bytecode
    /// Returns true if execution succeeded without running out of Prana
    bool execute(ContractState& state, state::StateDB* global_state = nullptr);

    /// Get remaining gas (Prana)
    uint64_t get_remaining_prana() const { return prana; }

private:
    std::vector<AgniInstruction> code;
    std::vector<std::string> stack; // String-based stack for simplicity
    uint64_t pc; // Program counter
    uint64_t prana; // Gas metering
};

} // namespace vm
} // namespace kasturisundari
