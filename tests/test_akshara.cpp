// Kasturisundari Chain — Phase 1: Akshara State Engine Test
#include "kasturisundari/state/state_db.h"
#include "kasturisundari/vm/evm_runner.h"
#include <iostream>
#include <cassert>

using namespace kasturisundari;

void test_akshara_write_once() {
    std::cout << "[Test Akshara] Running write-once pure storage test..." << std::endl;
    state::StateDB state;
    state.open(":memory:");

    std::string contract_addr = "0x416b736861726100000000000000000000000001"; // Starts with 0x416b
    std::string slot_key = "storage_0000000000000000000000000000000000000000000000000000000000000001";
    std::string initial_val = "0x0000000000000000000000000000000000000000000000000000000000001081";

    // 1. Initial write to empty slot must succeed
    bool first_write = state.set_contract_state(contract_addr, slot_key, initial_val);
    assert(first_write == true);
    assert(state.get_contract_state(contract_addr, slot_key) == initial_val);
    std::cout << "  ✓ Initial write succeeded: " << initial_val << std::endl;

    // 2. Overwrite attempt on Akshara slot must be REJECTED
    std::string overwrite_val = "0x0000000000000000000000000000000000000000000000000000000000009999";
    bool second_write = state.set_contract_state(contract_addr, slot_key, overwrite_val);
    assert(second_write == false);
    // Value must remain untouched!
    assert(state.get_contract_state(contract_addr, slot_key) == initial_val);
    std::cout << "  ✓ Overwrite blocked by Akshara Engine! Original value intact." << std::endl;

    // 3. Clear/deletion attempt must also be REJECTED
    std::string clear_val = "0x0000000000000000000000000000000000000000000000000000000000000000";
    bool clear_write = state.set_contract_state(contract_addr, slot_key, clear_val);
    assert(clear_write == false);
    assert(state.get_contract_state(contract_addr, slot_key) == initial_val);
    std::cout << "  ✓ Deletion/Zeroing blocked by Akshara Engine." << std::endl;
}

void test_selfdestruct_disabled() {
    std::cout << "[Test Akshara] Running SELFDESTRUCT opcode interception test..." << std::endl;
    state::StateDB state;
    state.open(":memory:");

    evmc::address sender = vm::hex_to_address("0x1111111111111111111111111111111111111111");
    evmc::address contract = vm::hex_to_address("0x2222222222222222222222222222222222222222");

    // SELFDESTRUCT opcode bytecode: 0xff
    std::vector<uint8_t> selfdestruct_bytecode = { 0xff };

    vm::EVMResult res = vm::execute_evm_tx(state, sender, contract, selfdestruct_bytecode, 0, 100000, 1, 1000, false);
    
    // Execution must revert or reject due to SELFDESTRUCT interception
    assert(res.status_code == EVMC_REVERT || res.status_code == EVMC_REJECTED || res.status_code == EVMC_FAILURE);
    std::cout << "  ✓ SELFDESTRUCT opcode (0xFF) intercepted and execution reverted!" << std::endl;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  KasturiChain Phase 1: Akshara Engine Test" << std::endl;
    std::cout << "==========================================" << std::endl;

    test_akshara_write_once();
    test_selfdestruct_disabled();

    std::cout << "\n✅ PHASE 1 AKSHARA ENGINE VERIFICATION PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
