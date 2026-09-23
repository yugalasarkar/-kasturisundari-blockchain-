// Kasturisundari Chain — Agni VM & StateDB Invariants Test Suite
// Rigorous verification for Phase 2: Sanskrit opcode to EVM bytecode translation,
// Stack/Memory bounds, and Gas Metering (Prana) out-of-gas reverts.

#include "kasturisundari/vm/agni_compiler.h"
#include "kasturisundari/vm/agni_machine.h"
#include "kasturisundari/state/state_db.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace kasturisundari;
using namespace kasturisundari::vm;

#define GREEN "\033[32m"
#define RED   "\033[31m"
#define CYAN  "\033[36m"
#define RESET "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    std::cout << CYAN "  [TEST] " RESET << name << "... " << std::flush;

#define PASS() \
    do { std::cout << GREEN "PASSED" RESET << std::endl; ++tests_passed; } while(0)

#define FAIL(msg) \
    do { std::cout << RED "FAILED: " << msg << RESET << std::endl; ++tests_failed; } while(0)

// ═══════════════════════════════════════════════════════════════════
// 1. Sanskrit Opcode to EVM Bytecode Translation
// ═══════════════════════════════════════════════════════════════════
void test_sanskrit_opcode_translation() {
    std::cout << "\n=== 1. Sanskrit Opcode to EVM Bytecode Translation ===" << std::endl;

    TEST("Translate 'वृद्धिरादैच्' -> OP_ALLOC");
    auto bc1 = AgniCompiler::compile("वृद्धिरादैच्");
    if (bc1.size() >= 1 && bc1[0].op == AgniOpcode::OP_ALLOC) {
        PASS();
    } else {
        FAIL("translation failed for vriddhiradaic");
    }

    TEST("Translate 'अकः सवर्णे दीर्घः' -> OP_ADD");
    auto bc2 = AgniCompiler::compile("अकः सवर्णे दीर्घः");
    if (bc2.size() >= 1 && bc2[0].op == AgniOpcode::OP_ADD) {
        PASS();
    } else {
        FAIL("translation failed for akah savarne dirghah");
    }

    TEST("Translate 'इको यणचि' -> OP_SUB");
    auto bc3 = AgniCompiler::compile("इको यणचि");
    if (bc3.size() >= 1 && bc3[0].op == AgniOpcode::OP_SUB) {
        PASS();
    } else {
        FAIL("translation failed for iko yanaci");
    }

    TEST("Compile multi-instruction Agni Bhasha script");
    std::string script = "वृद्धिरादैच् 100 50 अकः सवर्णे दीर्घः 30 इको यणचि";
    auto bc_full = AgniCompiler::compile(script);
    if (!bc_full.empty() && bc_full.back().op == AgniOpcode::OP_END) {
        PASS();
    } else {
        FAIL("multi-instruction script compilation failed");
    }
}

// ═══════════════════════════════════════════════════════════════════
// 2. Stack & Memory Bounds (Underflow / Overflow Safety)
// ═══════════════════════════════════════════════════════════════════
void test_stack_and_memory_bounds() {
    std::cout << "\n=== 2. Stack & Memory Bounds Protection ===" << std::endl;

    TEST("Stack underflow safety (OP_ADD on empty stack does NOT crash host)");
    AgniMachine vm_underflow(1000);
    ContractState state;
    std::vector<AgniInstruction> code_underflow = {
        {AgniOpcode::OP_ADD, ""},
        {AgniOpcode::OP_END, ""}
    };
    vm_underflow.load_bytecode(code_underflow);
    bool ok_underflow = vm_underflow.execute(state);
    if (!ok_underflow) {
        PASS(); // Gracefully failed execution on underflow
    } else {
        FAIL("stack underflow did not return false");
    }

    TEST("Stack underflow safety (OP_SUB on single-element stack)");
    AgniMachine vm_underflow2(1000);
    std::vector<AgniInstruction> code_underflow2 = {
        {AgniOpcode::OP_ALLOC, "100"},
        {AgniOpcode::OP_SUB, ""},
        {AgniOpcode::OP_END, ""}
    };
    vm_underflow2.load_bytecode(code_underflow2);
    bool ok_underflow2 = vm_underflow2.execute(state);
    if (!ok_underflow2) {
        PASS();
    } else {
        FAIL("single-element OP_SUB underflow was NOT caught");
    }
}

// ═══════════════════════════════════════════════════════════════════
// 3. Gas Metering (Prana) & State Revert on Out-Of-Gas
// ═══════════════════════════════════════════════════════════════════
void test_gas_metering_and_state_reverts() {
    std::cout << "\n=== 3. Gas Metering (Prana) & Out-of-Gas Reverts ===" << std::endl;

    TEST("Prana gas deduction during execution");
    AgniMachine vm_gas(100);
    ContractState state;
    std::vector<AgniInstruction> code = {
        {AgniOpcode::OP_ALLOC, "100"},
        {AgniOpcode::OP_ALLOC, "50"},
        {AgniOpcode::OP_ADD, ""},
        {AgniOpcode::OP_END, ""}
    };
    vm_gas.load_bytecode(code);
    if (vm_gas.execute(state) && vm_gas.get_remaining_prana() == 96) { // 100 - 4 instructions
        PASS();
    } else {
        FAIL("gas deduction calculation incorrect");
    }

    TEST("Out-of-Gas (0 Prana) halts execution and reverts cleanly");
    AgniMachine vm_oog(2); // Only 2 units of Prana
    ContractState state_before = state;
    vm_oog.load_bytecode(code); // Requires 4 units
    bool result = vm_oog.execute(state);

    if (!result && vm_oog.get_remaining_prana() == 0 && state == state_before) {
        PASS();
    } else {
        FAIL("out-of-gas did NOT revert state or halt cleanly");
    }
}

int main() {
    std::cout << "===============================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Agni VM Invariants Test Suite" << std::endl;
    std::cout << "===============================================================" << std::endl;

    test_sanskrit_opcode_translation();
    test_stack_and_memory_bounds();
    test_gas_metering_and_state_reverts();

    std::cout << "\n===============================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "===============================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
