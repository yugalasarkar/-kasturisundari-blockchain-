# ─── Kasturisundari Core Library ──────────────────────────────────────
find_package(OpenSSL REQUIRED)

add_library(kasturisundari_core STATIC
    crypto/sha256.cpp
    crypto/ripemd160.cpp
    crypto/secp256k1_wrapper.cpp
    crypto/address.cpp
    crypto/base58.cpp
    crypto/post_quantum.cpp
    attestation/text_attestation.cpp
    attestation/audio_attestation.cpp
    attestation/panini_engine.cpp
    vm/agni_compiler.cpp
    vm/agni_machine.cpp
    vm/evm_runner.cpp
    storage/vedic_storage.cpp
    governance/sabha.cpp
    core/transaction.cpp
    core/block.cpp
    core/blockchain.cpp
    state/state_db.cpp
    economics/tokenomics.cpp
    economics/pol_restaking.cpp
    genesis/genesis.cpp
    mempool/mempool.cpp
    poc/proof_of_contribution.cpp
    poc/poc_forger.cpp
    net/p2p_message.cpp
    net/p2p_node.cpp
    net/lora_serial.cpp
    net/aggkit_adapter.cpp
    rpc/rpc_server.cpp
)

target_include_directories(kasturisundari_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(kasturisundari_core PUBLIC
    evmone
    OpenSSL::Crypto
    OpenSSL::SSL
)
