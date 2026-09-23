// Kasturisundari Chain — Agni VM Implementation

#include "kasturisundari/vm/agni_machine.h"
#include <iostream>

namespace kasturisundari {
namespace vm {

AgniMachine::AgniMachine(uint64_t initial_prana) : pc(0), prana(initial_prana) {}

void AgniMachine::load_bytecode(const std::vector<AgniInstruction>& bytecode) {
    code = bytecode;
    pc = 0;
    stack.clear();
}

bool AgniMachine::execute(ContractState& state, state::StateDB* global_state) {
    while (pc < code.size()) {
        if (prana == 0) {
            std::cerr << "[Agni VM] Out of Prana (Gas Depleted)!\n";
            return false;
        }

        AgniInstruction inst = code[pc++];
        prana--; // Consume 1 Prana per opcode

        // If it's OP_UNKNOWN, we treat it as a PUSH operation of the argument
        if (inst.op == AgniOpcode::OP_UNKNOWN) {
            stack.push_back(inst.arg);
            continue;
        }

        switch (inst.op) {
            case AgniOpcode::OP_ALLOC:
                // वृद्धिरादैच् (Allocation / Variable setup)
                stack.push_back("0");
                break;
                
            case AgniOpcode::OP_ADD: {
                // अकः सवर्णे दीर्घः (Addition/Concat)
                if (stack.size() < 2) return false;
                std::string b = stack.back(); stack.pop_back();
                std::string a = stack.back(); stack.pop_back();
                
                // Try numeric addition, else fallback to string concat
                try {
                    uint64_t num_a = std::stoull(a);
                    uint64_t num_b = std::stoull(b);
                    stack.push_back(std::to_string(num_a + num_b));
                } catch (...) {
                    stack.push_back(a + b);
                }
                break;
            }
                
            case AgniOpcode::OP_SUB: {
                // इको यणचि (Subtraction)
                if (stack.size() < 2) return false;
                std::string b = stack.back(); stack.pop_back();
                std::string a = stack.back(); stack.pop_back();
                try {
                    uint64_t num_a = std::stoull(a);
                    uint64_t num_b = std::stoull(b);
                    stack.push_back(std::to_string(num_a >= num_b ? num_a - num_b : 0));
                } catch (...) {
                    return false; // Type mismatch
                }
                break;
            }
                
            case AgniOpcode::FUNC_WRITE_STATE: {
                // रक्ष् (Save to state)
                if (stack.size() < 2) return false;
                std::string val = stack.back(); stack.pop_back();
                std::string key = stack.back(); stack.pop_back();
                state[key] = val;
                prana -= 10; // State writes cost more
                break;
            }
                
            case AgniOpcode::FUNC_READ_STATE: {
                // ज्ञा (Read state)
                if (stack.size() < 1) return false;
                std::string key = stack.back(); stack.pop_back();
                if (state.find(key) != state.end()) {
                    stack.push_back(state[key]);
                } else {
                    stack.push_back("0");
                }
                prana -= 2;
                break;
            }
                
            case AgniOpcode::FUNC_TRANSFER: {
                // दा (Give/Transfer)
                if (stack.size() < 3) return false;
                std::string amount_str = stack.back(); stack.pop_back();
                std::string recipient = stack.back(); stack.pop_back();
                std::string sender = stack.back(); stack.pop_back();
                
                if (global_state) {
                    try {
                        uint64_t amount = std::stoull(amount_str);
                        state::Account sender_acc = global_state->get_account(sender);
                        state::Account rec_acc = global_state->get_account(recipient);
                        if (sender_acc.balance >= amount) {
                            sender_acc.balance -= amount;
                            rec_acc.balance += amount;
                            global_state->put_account(sender, sender_acc);
                            global_state->put_account(recipient, rec_acc);
                        } else {
                            return false; // Insufficient balance
                        }
                    } catch (...) {
                        return false;
                    }
                }
                prana -= 50; // Transfers are expensive
                break;
            }

            case AgniOpcode::OP_RETURN:
            case AgniOpcode::OP_END:
                return true; // Graceful halt
                
            default:
                // Ignored or unimplemented opcodes
                break;
        }
    }
    return true;
}

} // namespace vm
} // namespace kasturisundari
