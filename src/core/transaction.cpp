// Kasturisundari Chain — Transaction Implementation

#include "kasturisundari/core/transaction.h"
#include "kasturisundari/crypto/address.h"

#include <chrono>
#include <cstring>

namespace kasturisundari {
namespace core {

const char* tx_type_to_string(TxType type) {
    switch (type) {
        case TxType::TRANSFER:              return "Transfer";
        case TxType::ATTESTATION:           return "Attestation";
        case TxType::CONTRIBUTION_PROPOSAL: return "ContributionProposal";
        case TxType::CONTRIBUTION_APPROVAL: return "ContributionApproval";
        case TxType::FEE_RECYCLE:           return "FeeRecycle";
        default:                            return "Unknown";
    }
}

namespace {

void write_u8(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

void write_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val >> 24));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val));
}

void write_u64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 7; i >= 0; --i)
        buf.push_back(static_cast<uint8_t>(val >> (i * 8)));
}

void write_string(std::vector<uint8_t>& buf, const std::string& s) {
    write_u32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

void write_bytes(std::vector<uint8_t>& buf,
                 const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

void write_vec(std::vector<uint8_t>& buf, const std::vector<uint8_t>& v) {
    write_u32(buf, static_cast<uint32_t>(v.size()));
    buf.insert(buf.end(), v.begin(), v.end());
}

uint8_t read_u8(const uint8_t*& p) { return *p++; }

uint32_t read_u32(const uint8_t*& p) {
    uint32_t v = (static_cast<uint32_t>(p[0]) << 24)
               | (static_cast<uint32_t>(p[1]) << 16)
               | (static_cast<uint32_t>(p[2]) << 8) | p[3];
    p += 4; return v;
}

uint64_t read_u64(const uint8_t*& p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    p += 8; return v;
}

std::string read_string(const uint8_t*& p) {
    uint32_t len = read_u32(p);
    std::string s(reinterpret_cast<const char*>(p), len);
    p += len; return s;
}

std::vector<uint8_t> read_vec(const uint8_t*& p) {
    uint32_t len = read_u32(p);
    std::vector<uint8_t> v(p, p + len);
    p += len; return v;
}

} // anonymous namespace

std::vector<uint8_t> Transaction::serialize_for_signing() const {
    std::vector<uint8_t> buf;
    buf.reserve(256);

    write_u32(buf, version);
    write_u8(buf, static_cast<uint8_t>(type));
    write_u64(buf, timestamp);
    write_u64(buf, nonce);
    write_string(buf, sender);
    write_string(buf, recipient);
    write_u64(buf, amount);
    write_u64(buf, fee);
    write_vec(buf, data);

    return buf;
}

crypto::Hash256 Transaction::compute_hash() const {
    auto buf = serialize_for_signing();
    return crypto::double_sha256(buf);
}

std::vector<uint8_t> Transaction::serialize() const {
    auto buf = serialize_for_signing();

    // Append public key (33 bytes).
    write_bytes(buf, sender_pubkey.data(), sender_pubkey.size());

    // Append signature (64 bytes).
    write_bytes(buf, signature.data(), signature.size());

    return buf;
}

Transaction Transaction::deserialize(const std::vector<uint8_t>& data) {
    Transaction tx;
    const uint8_t* p = data.data();

    tx.version   = read_u32(p);
    tx.type      = static_cast<TxType>(read_u8(p));
    tx.timestamp = read_u64(p);
    tx.nonce     = read_u64(p);
    tx.sender    = read_string(p);
    tx.recipient = read_string(p);
    tx.amount    = read_u64(p);
    tx.fee       = read_u64(p);
    tx.data      = read_vec(p);

    // Public key (33 bytes).
    std::memcpy(tx.sender_pubkey.data(), p, 33);
    p += 33;

    // Signature (64 bytes).
    std::memcpy(tx.signature.data(), p, 64);
    p += 64;

    return tx;
}

bool Transaction::sign_transaction(const crypto::PrivateKey& privkey) {
    // Derive the public key.
    auto pubkey_opt = crypto::derive_public_key(privkey);
    if (!pubkey_opt) return false;
    sender_pubkey = pubkey_opt.value();

    // Compute the transaction hash.
    crypto::Hash256 hash = compute_hash();

    // Sign it.
    auto sig_opt = crypto::sign(privkey, hash);
    if (!sig_opt) return false;
    signature = sig_opt.value();

    return true;
}

bool Transaction::verify_signature() const {
    crypto::Hash256 hash = compute_hash();
    if (!crypto::verify(sender_pubkey, hash, signature)) {
        return false;
    }
    // SECURE: Ensure the derived address from pubkey actually matches the sender address!
    std::string derived_address = crypto::public_key_to_address(sender_pubkey);
    if (sender != derived_address) {
        return false;
    }
    return true;
}

std::string Transaction::txid() const {
    return crypto::hash_to_hex(compute_hash());
}

size_t transaction_size(const Transaction& tx) {
    return tx.serialize().size();
}

} // namespace core
} // namespace kasturisundari
