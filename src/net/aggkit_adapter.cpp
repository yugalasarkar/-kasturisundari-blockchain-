// Kasturisundari Chain — AggKit LxLy Bridge Adapter & Pessimistic Proof Engine Implementation

#include "kasturisundari/net/aggkit_adapter.h"
#include <iostream>
#include <sstream>

namespace kasturisundari {
namespace net {

crypto::Hash256 LxLyBridgeMessage::compute_leaf_hash() const {
    std::vector<uint8_t> buf;
    buf.reserve(256);
    
    // Serialize message fields
    for (int i = 3; i >= 0; --i) buf.push_back((origin_network >> (i * 8)) & 0xFF);
    buf.insert(buf.end(), origin_address.begin(), origin_address.end());
    for (int i = 3; i >= 0; --i) buf.push_back((destination_network >> (i * 8)) & 0xFF);
    buf.insert(buf.end(), destination_address.begin(), destination_address.end());
    for (int i = 7; i >= 0; --i) buf.push_back((amount >> (i * 8)) & 0xFF);
    for (int i = 7; i >= 0; --i) buf.push_back((deposit_count >> (i * 8)) & 0xFF);
    buf.insert(buf.end(), payload.begin(), payload.end());

    return crypto::double_sha256(buf);
}

AggKitAdapter::AggKitAdapter(state::StateDB& state) : state_(state) {}

crypto::Hash256 AggKitAdapter::compute_merkle_tree_root(const std::vector<crypto::Hash256>& leaves) const {
    if (leaves.empty()) return crypto::Hash256{};
    if (leaves.size() == 1) return leaves[0];

    std::vector<crypto::Hash256> current = leaves;
    while (current.size() > 1) {
        if (current.size() % 2 != 0) {
            current.push_back(current.back());
        }
        std::vector<crypto::Hash256> next_level;
        for (size_t i = 0; i < current.size(); i += 2) {
            std::vector<uint8_t> combined;
            combined.insert(combined.end(), current[i].begin(), current[i].end());
            combined.insert(combined.end(), current[i+1].begin(), current[i+1].end());
            next_level.push_back(crypto::double_sha256(combined));
        }
        current = next_level;
    }
    return current[0];
}

bool AggKitAdapter::bridge_and_call(uint32_t dest_network,
                                    const std::string& sender,
                                    const std::string& recipient,
                                    uint64_t amount,
                                    const std::vector<uint8_t>& payload,
                                    crypto::Hash256& out_leaf_hash) {
    std::lock_guard<std::mutex> lock(mtx_);

    // Check sender balance
    uint64_t bal = state_.get_balance(sender);
    if (bal < amount) {
        std::cerr << "[AggKit Adapter] Bridge rejected: Insufficient balance for sender=" << sender << "\n";
        return false;
    }

    // Deduct balance from sender
    state::Account acc = state_.get_account(sender);
    acc.balance -= amount;
    state_.put_account(sender, acc);

    // Build LxLy Bridge Message
    LxLyBridgeMessage msg;
    msg.origin_network = 108108; // KasturiChain
    msg.origin_address = sender;
    msg.destination_network = dest_network;
    msg.destination_address = recipient;
    msg.amount = amount;
    msg.payload = payload;
    msg.deposit_count = ++deposit_count_;

    out_leaf_hash = msg.compute_leaf_hash();
    exit_leaves_.push_back(out_leaf_hash);
    total_deposited_amount_ += amount;

    current_local_exit_root_ = compute_merkle_tree_root(exit_leaves_);
    current_global_exit_root_ = current_local_exit_root_;

    std::cout << "[AggKit Adapter] bridgeAndCall executed. Leaf: " 
              << crypto::hash_to_hex(out_leaf_hash) 
              << " | LocalExitRoot: " << crypto::hash_to_hex(current_local_exit_root_) << "\n";

    return true;
}

AggKitPessimisticProof AggKitAdapter::generate_pessimistic_proof() const {
    std::lock_guard<std::mutex> lock(mtx_);

    AggKitPessimisticProof proof;
    proof.network_id = 108108;
    proof.local_exit_root = current_local_exit_root_;
    proof.global_exit_root = current_global_exit_root_;
    proof.total_deposits = total_deposited_amount_;
    proof.total_withdrawals = total_withdrawn_amount_;
    proof.exit_tree_proof = exit_leaves_;

    return proof;
}

bool AggKitAdapter::submit_pessimistic_proof(const AggKitPessimisticProof& proof) {
    std::lock_guard<std::mutex> lock(mtx_);

    // Enforce pessimistic security invariant: Total Withdrawals <= Total Deposits
    if (!proof.verify_pessimistic_invariant()) {
        std::cerr << "[AggKit Pessimistic Proof Engine] REJECTED: Total withdrawals (" 
                  << proof.total_withdrawals << ") exceed total deposits (" 
                  << proof.total_deposits << ")!\n";
        return false;
    }

    current_global_exit_root_ = proof.global_exit_root;
    std::cout << "[AggKit Pessimistic Proof Engine] Verified & Accepted Pessimistic Proof. GlobalExitRoot: " 
              << crypto::hash_to_hex(current_global_exit_root_) << "\n";

    return true;
}

crypto::Hash256 AggKitAdapter::get_global_exit_root() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return current_global_exit_root_;
}

} // namespace net
} // namespace kasturisundari
