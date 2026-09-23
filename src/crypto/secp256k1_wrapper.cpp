// Kasturisundari Chain — secp256k1 Elliptic Curve Wrapper
// Production-grade implementation using OpenSSL with RFC 6979 deterministic nonces,
// Low-S malleability protection, and public key recovery.

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "kasturisundari/crypto/secp256k1_wrapper.h"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace kasturisundari {
namespace crypto {

namespace {

void hmac_sha256(const uint8_t* key, size_t key_len,
                 const uint8_t* data, size_t data_len,
                 uint8_t* out) {
    unsigned int out_len = 32;
    HMAC(EVP_sha256(), key, key_len, data, data_len, out, &out_len);
}

// RFC 6979 Section 3.2 HMAC-SHA256 Deterministic Nonce Derivation
BIGNUM* generate_rfc6979_k(const BIGNUM* priv_bn, const Hash256& msg_hash, const BIGNUM* order) {
    uint8_t x_bytes[32];
    BN_bn2binpad(priv_bn, x_bytes, 32);

    uint8_t v[32];
    uint8_t k[32];
    std::memset(v, 0x01, 32);
    std::memset(k, 0x00, 32);

    std::vector<uint8_t> blob;
    blob.reserve(32 + 1 + 32 + 32);

    // Step a: K = HMAC_K(V || 0x00 || int2octets(x) || bits2octets(h))
    blob.insert(blob.end(), v, v + 32);
    blob.push_back(0x00);
    blob.insert(blob.end(), x_bytes, x_bytes + 32);
    blob.insert(blob.end(), msg_hash.begin(), msg_hash.end());
    hmac_sha256(k, 32, blob.data(), blob.size(), k);

    // Step b: V = HMAC_K(V)
    hmac_sha256(k, 32, v, 32, v);

    // Step c: K = HMAC_K(V || 0x01 || int2octets(x) || bits2octets(h))
    blob.clear();
    blob.insert(blob.end(), v, v + 32);
    blob.push_back(0x01);
    blob.insert(blob.end(), x_bytes, x_bytes + 32);
    blob.insert(blob.end(), msg_hash.begin(), msg_hash.end());
    hmac_sha256(k, 32, blob.data(), blob.size(), k);

    // Step d: V = HMAC_K(V)
    hmac_sha256(k, 32, v, 32, v);

    BIGNUM* k_bn = BN_new();
    while (true) {
        hmac_sha256(k, 32, v, 32, v);
        BN_bin2bn(v, 32, k_bn);
        if (BN_cmp(k_bn, BN_value_one()) >= 0 && BN_cmp(k_bn, order) < 0) {
            break;
        }
        blob.clear();
        blob.insert(blob.end(), v, v + 32);
        blob.push_back(0x00);
        hmac_sha256(k, 32, blob.data(), blob.size(), k);
        hmac_sha256(k, 32, v, 32, v);
    }

    return k_bn;
}

uint8_t hex_char_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0xFF;
}

} // un-named namespace

PrivateKey generate_private_key() {
    PrivateKey priv;
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BIGNUM* order = BN_new();
    BIGNUM* bn_priv = BN_new();
    EC_GROUP_get_order(group, order, NULL);

    do {
        RAND_bytes(priv.data(), 32);
        BN_bin2bn(priv.data(), 32, bn_priv);
    } while (BN_is_zero(bn_priv) || BN_cmp(bn_priv, order) >= 0);

    BN_free(bn_priv);
    BN_free(order);
    EC_GROUP_free(group);
    return priv;
}

std::optional<PublicKey> derive_public_key(const PrivateKey& privkey) {
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BIGNUM* order = BN_new();
    BIGNUM* bn_priv = BN_bin2bn(privkey.data(), 32, NULL);
    EC_GROUP_get_order(group, order, NULL);

    if (BN_is_zero(bn_priv) || BN_cmp(bn_priv, order) >= 0) {
        BN_free(bn_priv);
        BN_free(order);
        EC_GROUP_free(group);
        return std::nullopt;
    }

    EC_POINT* point = EC_POINT_new(group);
    BN_CTX* ctx = BN_CTX_new();

    if (!EC_POINT_mul(group, point, bn_priv, NULL, NULL, ctx)) {
        EC_POINT_free(point);
        BN_CTX_free(ctx);
        BN_free(bn_priv);
        BN_free(order);
        EC_GROUP_free(group);
        return std::nullopt;
    }

    PublicKey pub;
    size_t len = EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, pub.data(), 33, ctx);

    EC_POINT_free(point);
    BN_CTX_free(ctx);
    BN_free(bn_priv);
    BN_free(order);
    EC_GROUP_free(group);

    if (len != 33) return std::nullopt;
    return pub;
}

std::optional<Signature> sign_with_recovery(const PrivateKey& privkey,
                                            const Hash256& msg_hash,
                                            uint8_t& out_v) {
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* order = BN_new();
    EC_GROUP_get_order(group, order, NULL);

    BIGNUM* priv_bn = BN_bin2bn(privkey.data(), 32, NULL);
    BIGNUM* h_bn = BN_bin2bn(msg_hash.data(), 32, NULL);

    if (BN_is_zero(priv_bn) || BN_cmp(priv_bn, order) >= 0) {
        BN_free(h_bn);
        BN_free(priv_bn);
        BN_free(order);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return std::nullopt;
    }

    BIGNUM* half_order = BN_new();
    BN_rshift1(half_order, order);

    BIGNUM* k_bn = generate_rfc6979_k(priv_bn, msg_hash, order);

    // Compute R = k * G
    EC_POINT* R_pt = EC_POINT_new(group);
    EC_POINT_mul(group, R_pt, k_bn, NULL, NULL, ctx);

    BIGNUM* r_bn = BN_new();
    BIGNUM* y_bn = BN_new();
    EC_POINT_get_affine_coordinates(group, R_pt, r_bn, y_bn, ctx);
    BN_nnmod(r_bn, r_bn, order, ctx);

    // Compute s = k^-1 * (h + r * priv) mod order
    BIGNUM* k_inv = BN_mod_inverse(NULL, k_bn, order, ctx);
    BIGNUM* tmp = BN_new();
    BN_mod_mul(tmp, r_bn, priv_bn, order, ctx);
    BN_mod_add(tmp, h_bn, tmp, order, ctx);

    BIGNUM* s_bn = BN_new();
    BN_mod_mul(s_bn, k_inv, tmp, order, ctx);

    uint8_t rec_id = (BN_is_odd(y_bn) ? 1 : 0);

    // Enforce canonical Low-S constraint (s <= n/2)
    if (BN_cmp(s_bn, half_order) > 0) {
        BN_sub(s_bn, order, s_bn);
        rec_id ^= 1;
    }

    Signature sig;
    BN_bn2binpad(r_bn, sig.data(), 32);
    BN_bn2binpad(s_bn, sig.data() + 32, 32);
    out_v = rec_id;

    BN_free(s_bn);
    BN_free(tmp);
    BN_free(k_inv);
    BN_free(y_bn);
    BN_free(r_bn);
    EC_POINT_free(R_pt);
    BN_free(k_bn);
    BN_free(half_order);
    BN_free(h_bn);
    BN_free(priv_bn);
    BN_free(order);
    BN_CTX_free(ctx);
    EC_GROUP_free(group);

    return sig;
}

std::optional<Signature> sign(const PrivateKey& privkey, const Hash256& msg_hash) {
    uint8_t dummy_v = 0;
    return sign_with_recovery(privkey, msg_hash, dummy_v);
}

bool verify(const PublicKey& pubkey, const Hash256& msg_hash, const Signature& sig) {
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* order = BN_new();
    EC_GROUP_get_order(group, order, NULL);

    BIGNUM* r_bn = BN_bin2bn(sig.data(), 32, NULL);
    BIGNUM* s_bn = BN_bin2bn(sig.data() + 32, 32, NULL);

    BIGNUM* half_order = BN_new();
    BN_rshift1(half_order, order);

    // Strict Low-S check: r != 0, r < order, s != 0, s <= half_order
    if (BN_is_zero(r_bn) || BN_cmp(r_bn, order) >= 0 ||
        BN_is_zero(s_bn) || BN_cmp(s_bn, half_order) > 0) {
        BN_free(half_order);
        BN_free(s_bn);
        BN_free(r_bn);
        BN_free(order);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return false;
    }

    EC_POINT* pub_pt = EC_POINT_new(group);
    if (!EC_POINT_oct2point(group, pub_pt, pubkey.data(), 33, ctx)) {
        EC_POINT_free(pub_pt);
        BN_free(half_order);
        BN_free(s_bn);
        BN_free(r_bn);
        BN_free(order);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return false;
    }

    ECDSA_SIG* ecdsa_sig = ECDSA_SIG_new();
    ECDSA_SIG_set0(ecdsa_sig, r_bn, s_bn); // Ownership of r_bn, s_bn transferred to ecdsa_sig

    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_set_public_key(ec_key, pub_pt);

    int res = ECDSA_do_verify(msg_hash.data(), 32, ecdsa_sig, ec_key);

    EC_KEY_free(ec_key);
    ECDSA_SIG_free(ecdsa_sig); // Frees r_bn and s_bn
    EC_POINT_free(pub_pt);
    BN_free(half_order);
    BN_free(order);
    BN_CTX_free(ctx);
    EC_GROUP_free(group);

    return res == 1;
}

std::optional<PublicKey> recover_public_key(const Hash256& msg_hash,
                                            const Signature& sig,
                                            uint8_t v) {
    uint8_t rec_id = 255;
    if (v == 0 || v == 1) {
        rec_id = v;
    } else if (v == 27 || v == 28) {
        rec_id = v - 27;
    } else {
        return std::nullopt; // Strictly reject invalid or out-of-bounds recovery IDs (e.g. 2, 3, 4..26, 29..255)
    }

    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* order = BN_new();
    EC_GROUP_get_order(group, order, NULL);

    BIGNUM* r_bn = BN_bin2bn(sig.data(), 32, NULL);
    BIGNUM* s_bn = BN_bin2bn(sig.data() + 32, 32, NULL);

    if (BN_is_zero(r_bn) || BN_cmp(r_bn, order) >= 0 ||
        BN_is_zero(s_bn) || BN_cmp(s_bn, order) >= 0) {
        BN_free(s_bn);
        BN_free(r_bn);
        BN_free(order);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return std::nullopt;
    }

    uint8_t r_compressed[33];
    r_compressed[0] = (rec_id & 1) ? 0x03 : 0x02;
    std::memcpy(r_compressed + 1, sig.data(), 32);

    EC_POINT* R_pt = EC_POINT_new(group);
    if (!EC_POINT_oct2point(group, R_pt, r_compressed, 33, ctx)) {
        EC_POINT_free(R_pt);
        BN_free(s_bn);
        BN_free(r_bn);
        BN_free(order);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return std::nullopt;
    }

    // Compute Q = r^-1 * (s * R - e * G)
    BIGNUM* r_inv = BN_mod_inverse(NULL, r_bn, order, ctx);
    BIGNUM* e_bn = BN_bin2bn(msg_hash.data(), 32, NULL);
    BN_nnmod(e_bn, e_bn, order, ctx);

    BIGNUM* neg_e = BN_new();
    BN_sub(neg_e, order, e_bn);

    EC_POINT* Q_pt = EC_POINT_new(group);
    const EC_POINT* points[1] = { R_pt };
    const BIGNUM* scalars[1] = { s_bn };

    if (!EC_POINTs_mul(group, Q_pt, neg_e, 1, points, scalars, ctx) ||
        !EC_POINT_mul(group, Q_pt, NULL, Q_pt, r_inv, ctx)) {
        EC_POINT_free(Q_pt);
        EC_POINT_free(R_pt);
        BN_free(neg_e);
        BN_free(e_bn);
        BN_free(r_inv);
        BN_free(s_bn);
        BN_free(r_bn);
        BN_free(order);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return std::nullopt;
    }

    PublicKey pub;
    size_t len = EC_POINT_point2oct(group, Q_pt, POINT_CONVERSION_COMPRESSED, pub.data(), 33, ctx);

    EC_POINT_free(Q_pt);
    EC_POINT_free(R_pt);
    BN_free(neg_e);
    BN_free(e_bn);
    BN_free(r_inv);
    BN_free(s_bn);
    BN_free(r_bn);
    BN_free(order);
    BN_CTX_free(ctx);
    EC_GROUP_free(group);

    if (len != 33) return std::nullopt;
    return pub;
}

std::string privkey_to_hex(const PrivateKey& privkey) {
    std::ostringstream oss;
    for (uint8_t byte : privkey) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return oss.str();
}

std::string pubkey_to_hex(const PublicKey& pubkey) {
    std::ostringstream oss;
    for (uint8_t byte : pubkey) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return oss.str();
}

std::string sig_to_hex(const Signature& sig) {
    std::ostringstream oss;
    for (uint8_t byte : sig) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return oss.str();
}

std::optional<PrivateKey> hex_to_privkey(const std::string& hex) {
    if (hex.length() != 64) return std::nullopt;
    PrivateKey key;
    for (size_t i = 0; i < 32; ++i) {
        uint8_t high = hex_char_to_val(hex[2 * i]);
        uint8_t low = hex_char_to_val(hex[2 * i + 1]);
        if (high == 0xFF || low == 0xFF) return std::nullopt;
        key[i] = (high << 4) | low;
    }
    return key;
}

std::optional<PublicKey> hex_to_pubkey(const std::string& hex) {
    if (hex.length() != 66) return std::nullopt;
    PublicKey key;
    for (size_t i = 0; i < 33; ++i) {
        uint8_t high = hex_char_to_val(hex[2 * i]);
        uint8_t low = hex_char_to_val(hex[2 * i + 1]);
        if (high == 0xFF || low == 0xFF) return std::nullopt;
        key[i] = (high << 4) | low;
    }
    return key;
}

std::optional<Signature> hex_to_sig(const std::string& hex) {
    if (hex.length() != 128) return std::nullopt;
    Signature sig;
    for (size_t i = 0; i < 64; ++i) {
        uint8_t high = hex_char_to_val(hex[2 * i]);
        uint8_t low = hex_char_to_val(hex[2 * i + 1]);
        if (high == 0xFF || low == 0xFF) return std::nullopt;
        sig[i] = (high << 4) | low;
    }
    return sig;
}

} // namespace crypto
} // namespace kasturisundari

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
