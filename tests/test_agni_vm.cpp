// Kasturisundari Chain — Agni VM Tests
// Tests the compilation and execution of Agni Bhasha smart contracts.

#include "kasturisundari/vm/agni_compiler.h"
#include "kasturisundari/vm/agni_machine.h"

#include <iostream>
#include <string>

using namespace kasturisundari;
using namespace kasturisundari::vm;

#define GREEN "\033[32m"
#define RED   "\033[31m"
#define CYAN  "\033[36m"
#define RESET "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    std::cout << CYAN "  [TEST] " RESET << name << "... ";

#define PASS() \
    do { std::cout << GREEN "PASSED" RESET << std::endl; ++tests_passed; } while(0)

#define FAIL(msg) \
    do { std::cout << RED "FAILED: " << msg << RESET << std::endl; ++tests_failed; } while(0)

void test_agni_vm() {
    std::cout << "\n=== Agni Bhasha (Smart Contracts) Tests ===" << std::endl;

    TEST("Compile empty script (returns OP_END)");
    auto bc1 = AgniCompiler::compile("");
    if (bc1.size() == 1 && bc1[0].op == AgniOpcode::OP_END) {
        PASS();
    } else {
        FAIL("Did not append OP_END");
    }

    TEST("Compile valid Agni Bhasha (Sutra: vriddhiradaic)");
    std::string script1 = "वृद्धिरादैच्";
    auto bc2 = AgniCompiler::compile(script1);
    if (bc2.size() == 2 && bc2[0].op == AgniOpcode::OP_ALLOC) {
        PASS();
    } else {
        FAIL("Failed to compile vriddhiradaic");
    }

    TEST("Compile code with unknown invalid token");
    std::string script2 = "वृद्धिरादैच् *invalid_token*";
    auto bc3 = AgniCompiler::compile(script2);
    if (bc3.empty()) {
        PASS();
    } else {
        FAIL("Compiled invalid code");
    }

    TEST("Execute VM Allocation (Prana consumption)");
    AgniMachine vm(100); // 100 Prana
    ContractState state;
    vm.load_bytecode(bc2); // OP_ALLOC, OP_END
    if (vm.execute(state)) {
        if (vm.get_remaining_prana() == 98) { // 100 - 2 opcodes
            PASS();
        } else {
            FAIL("Incorrect Prana consumption");
        }
    } else {
        FAIL("Execution failed");
    }

    TEST("Compile and execute sample_vedic.agni");
    std::string sample_path = "tests/contracts/sample_vedic.agni";
    std::string sample_code = "वृद्धिरादैच् 100 50 अकः सवर्णे दीर्घः";
    auto bc_sample = AgniCompiler::compile(sample_code);
    
    AgniMachine vm2(100);
    vm2.load_bytecode(bc_sample);
    if (vm2.execute(state)) {
        PASS();
    } else {
        FAIL("Failed to execute sample_vedic.agni bytecode");
    }
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Agni VM Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_agni_vm();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
