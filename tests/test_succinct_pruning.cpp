// Kasturisundari Chain — Phase 3: Succinct State Pruning Test
#include "kasturisundari/core/blockchain.h"
#include "kasturisundari/genesis/genesis.h"
#include "kasturisundari/economics/tokenomics.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace kasturisundari;

void test_succinct_pruning_and_fast_bootstrap() {
    std::cout << "[Test Succinct Pruning] Running light sovereign fast bootstrap test..." << std::endl;

    core::Blockchain full_node;

    // 1. Build Genesis
    genesis::GenesisConfig cfg;
    cfg.founder_address = economics::FOUNDER_ADDRESS;
    cfg.secondary_address = economics::SECONDARY_ADDRESS;
    cfg.reward_pool_address = economics::REWARD_POOL_ADDRESS;
    cfg.timestamp = 1690000000;
    core::Block genesis_block = genesis::build_genesis_block(cfg);
    full_node.initialize_genesis(genesis_block);

    // 2. Mine 50 blocks on full node
    core::Block prev = genesis_block;
    for (uint64_t i = 1; i <= 50; ++i) {
        core::Block b;
        b.header.version = 1;
        b.header.height = i;
        b.header.previous_hash = prev.compute_hash();
        b.header.timestamp = prev.header.timestamp + 1;
        b.header.tx_count = 0;
        b.header.total_fees = 0;
        b.header.merkle_root = b.compute_merkle_root();

        auto res = full_node.add_block(b);
        assert(res == core::AddBlockResult::SUCCESS);
        prev = b;
    }

    assert(full_node.get_height() == 50);
    assert(full_node.block_count() == 51); // 50 + Genesis

    // 3. Generate Succinct State Proof
    auto start_time = std::chrono::high_resolution_clock::now();
    core::Blockchain::StateSnapshotProof state_proof = full_node.generate_state_proof();
    assert(state_proof.finalized_height == 50);
    assert(state_proof.header_hashes.size() == 51);

    // 4. Instantiate a brand new Light Sovereign Node (zero history stored)
    core::Blockchain light_sovereign_node;
    bool bootstrapped = light_sovereign_node.bootstrap_from_state_proof(state_proof, full_node.get_latest_block());
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start_time).count();

    assert(bootstrapped == true);
    assert(light_sovereign_node.is_light_sovereign() == true);
    assert(light_sovereign_node.get_height() == 50);
    assert(light_sovereign_node.block_count() == 1); // Only tip stored in active RAM!

    std::cout << "  ✓ Light sovereign node bootstrapped to height #50 in " << elapsed << " microseconds." << std::endl;

    // 5. Verify light node accepts block #51 directly on top of bootstrapped tip
    core::Block block51;
    block51.header.version = 1;
    block51.header.height = 51;
    block51.header.previous_hash = light_sovereign_node.get_latest_block().compute_hash();
    block51.header.timestamp = light_sovereign_node.get_latest_block().header.timestamp + 1;
    block51.header.tx_count = 0;
    block51.header.total_fees = 0;
    block51.header.merkle_root = block51.compute_merkle_root();

    auto add_res = light_sovereign_node.add_block(block51);
    assert(add_res == core::AddBlockResult::SUCCESS);
    assert(light_sovereign_node.get_height() == 51);

    std::cout << "  ✓ Light sovereign node successfully validated new block #51 on top of pruned state!" << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "  KasturiChain Phase 3: Succinct State Pruning Test " << std::endl;
    std::cout << "=====================================================" << std::endl;

    test_succinct_pruning_and_fast_bootstrap();

    std::cout << "\n✅ PHASE 3 SUCCINCT STATE PRUNING PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
