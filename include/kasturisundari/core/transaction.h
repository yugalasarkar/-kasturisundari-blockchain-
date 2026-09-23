// Kasturisundari Chain — Transaction Structure
// Defines the fundamental unit of value transfer on the network.
//
// Uses an Account-based model (like Ethereum) rather than UTXO,
// which is more natural for smart contracts and Proof of Contribution.

#pragma once

#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include "kasturisundari/crypto/sha256.h"

#include <cstdint>
#include <string>
#include <vector>

namespace kasturisundari {
namespace core {

/// Transaction types supported by Kasturisundari Chain.
enum class TxType : uint8_t {
    TRANSFER              = 0x01,  // Standard NSK transfer
    ATTESTATION           = 0x02,  // Sacred text attestation
    CONTRIBUTION_PROPOSAL = 0x03,  // Proof of Contribution submission
    CONTRIBUTION_APPROVAL = 0x04,  // Approve a contribution (releases reward)
    FEE_RECYCLE           = 0x05,  // System tx: recycle fees back to reward pool
    CONTRACT_DEPLOY       = 0x06,  // Deploy a smart contract
    CONTRACT_CALL         = 0x07,  // Execute a smart contract function
    GOVERNANCE_VOTE       = 0x08   // Vote on a Vedic Sabha proposal
};

/// Convert TxType to a human-readable string.
const char* tx_type_to_string(TxType type);

/// A single transaction on the Kasturisundari Chain.
struct Transaction {
    // ─── Header Fields ──────────────────────────────────────────
    uint32_t    version;        // Protocol version (currently 1)
    TxType      type;           // Transaction type
    uint64_t    timestamp;      // Unix timestamp (seconds)
    uint64_t    nonce;          // Sender's sequence number (prevents replay)

    // ─── Transfer Fields ────────────────────────────────────────
    std::string sender;         // Sender address (K-prefixed)
    std::string recipient;      // Recipient address (K-prefixed)
    uint64_t    amount;         // Amount in smallest unit (1e-8 NSK)
    uint64_t    fee;            // Transaction fee in smallest unit

    // ─── Data Payload & Dharma Anti-MEV Commitments ────────────
    std::vector<uint8_t> data;  // Arbitrary data (attestation hash, contribution ref, etc.)
    bool     is_encrypted = false;         // Indicates time-locked encrypted mempool payload
    crypto::Hash256 commitment_hash{};    // Commitment hash of payload sha256(data + salt)
    uint64_t arrival_timestamp_ns = 0;     // High-precision FCFS arrival timestamp for Dharma ordering

    // ─── Signature ──────────────────────────────────────────────
    crypto::PublicKey  sender_pubkey;   // Sender's compressed public key
    crypto::Signature  signature;       // ECDSA signature of the transaction hash

    /// Compute the hash of this transaction (excluding the signature).
    /// This is the message that gets signed.
    crypto::Hash256 compute_hash() const;

    /// Serialize the transaction to binary (for hashing / storage).
    std::vector<uint8_t> serialize() const;

    /// Serialize only the signable portion (everything except signature).
    std::vector<uint8_t> serialize_for_signing() const;

    /// Deserialize a transaction from binary data.
    static Transaction deserialize(const std::vector<uint8_t>& data);

    /// Sign this transaction with a private key.
    /// Sets the sender_pubkey and signature fields.
    bool sign_transaction(const crypto::PrivateKey& privkey);

    /// Verify the signature of this transaction.
    bool verify_signature() const;

    /// Get the transaction ID (its hash as a hex string).
    std::string txid() const;
};

/// Compute the total size of a serialized transaction in bytes.
size_t transaction_size(const Transaction& tx);

} // namespace core
} // namespace kasturisundari
