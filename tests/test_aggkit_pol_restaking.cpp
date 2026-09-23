// Kasturisundari Chain — AggKit LxLy Adapter & POL Dual-Restaking Integration Test
#include "kasturisundari/net/aggkit_adapter.h"
#include "kasturisundari/economics/pol_restaking.h"
#include "kasturisundari/state/state_db.h"
#include <iostream>
#include <cassert>

using namespace kasturisundari;

void test_aggkit_pessimistic_proof_bridge() {
    std::cout << "[Test AggKit Adapter] Running LxLy Unified Bridge & Pessimistic Proof Test..." << std::endl;

    state::StateDB state;
    state.open(":memory:");

    std::string sender = "K1111111111111111111111111111111111111111";
    state::Account acc{ 500000000, 0 };
    state.put_account(sender, acc);

    net::AggKitAdapter adapter(state);

    // 1. Execute bridgeAndCall deposit
    crypto::Hash256 leaf_hash{};
    std::vector<uint8_t> payload = { 0x01, 0x02, 0x03, 0x04 };
    bool deposit_ok = adapter.bridge_and_call(1, sender, "0x9999999999999999999999999999999999999999", 1000000, payload, leaf_hash);
    assert(deposit_ok == true);
    assert(adapter.get_deposit_count() == 1);
    std::cout << "  ✓ bridgeAndCall executed. Leaf hash: " << crypto::hash_to_hex(leaf_hash) << std::endl;

    // 2. Generate Pessimistic Proof
    net::AggKitPessimisticProof proof = adapter.generate_pessimistic_proof();
    assert(proof.total_deposits == 1000000);
    assert(proof.total_withdrawals == 0);
    assert(proof.verify_pessimistic_invariant() == true);
    std::cout << "  ✓ Valid Pessimistic Proof generated: Total Deposits = 1000000, Total Withdrawals = 0." << std::endl;

    // 3. Verify valid proof submission
    bool proof_ok = adapter.submit_pessimistic_proof(proof);
    assert(proof_ok == true);
    std::cout << "  ✓ Pessimistic Proof accepted by AggKit Engine." << std::endl;

    // 4. Test Over-withdrawal invariant violation (Total Withdrawals > Total Deposits)
    net::AggKitPessimisticProof fraudulent_proof = proof;
    fraudulent_proof.total_withdrawals = 2000000; // Over-withdrawal attempt!
    assert(fraudulent_proof.verify_pessimistic_invariant() == false);
    
    bool fraud_ok = adapter.submit_pessimistic_proof(fraudulent_proof);
    assert(fraud_ok == false);
    std::cout << "  ✓ Fraudulent Over-withdrawal attempt successfully REJECTED by Pessimistic Proof Engine!" << std::endl;
}

void test_pol_dual_restaking_vault() {
    std::cout << "[Test POL Restaking Vault] Running POL + NILA Dual-Staking Test..." << std::endl;

    state::StateDB state;
    state.open(":memory:");

    std::string valA = "K2222222222222222222222222222222222222222";
    state::Account accA{ 1000000000, 0 };
    state.put_account(valA, accA);

    economics::PolRestakingVault vault(state);

    // 1. Stake POL
    bool pol_staked = vault.stake_pol(valA, 500000);
    assert(pol_staked == true);
    assert(vault.get_total_pol_restaked() == 500000);

    // 2. Stake NILA
    bool nila_staked = vault.stake_nila(valA, 300000);
    assert(nila_staked == true);
    assert(vault.get_total_nila_staked() == 300000);

    // 3. Verify effective weight calculation (NILA + POL * ExchangeRate)
    economics::ValidatorStakeRecord rec = vault.get_validator_stake(valA);
    assert(rec.staked_pol == 500000);
    assert(rec.staked_nila == 300000);
    assert(rec.get_effective_weight() == 800000);
    std::cout << "  ✓ Effective validator weight calculated: " << rec.get_effective_weight() << std::endl;

    // 4. Test fee reward distribution
    vault.distribute_fee_rewards(10000);
    economics::ValidatorStakeRecord updated_rec = vault.get_validator_stake(valA);
    assert(updated_rec.reward_accumulated_pol == 10000);
    std::cout << "  ✓ Fee rewards distributed to POL restaker: " << updated_rec.reward_accumulated_pol << std::endl;

    // 5. Test Slashing
    bool slashed = vault.slash_validator(valA, 0.50); // 50% slash
    assert(slashed == true);
    economics::ValidatorStakeRecord slashed_rec = vault.get_validator_stake(valA);
    assert(slashed_rec.is_slashed == true);
    assert(slashed_rec.get_effective_weight() == 0); // Effective weight drops to 0 after slashing
    std::cout << "  ✓ Slashing enforced. Validator effective weight dropped to 0." << std::endl;
}

int main() {
    std::cout << "==========================================================================" << std::endl;
    std::cout << "  KasturiChain AggKit LxLy Adapter & POL Dual-Restaking Integration Test " << std::endl;
    std::cout << "==========================================================================" << std::endl;

    test_aggkit_pessimistic_proof_bridge();
    test_pol_dual_restaking_vault();

    std::cout << "\n✅ ALL AGGKIT & POL DUAL-RESTAKING INTEGRATION TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
