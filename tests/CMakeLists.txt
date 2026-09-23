# ─── Tests ────────────────────────────────────────────────────────────
add_executable(test_crypto test_crypto.cpp)
target_link_libraries(test_crypto PRIVATE kasturisundari_core)
add_test(NAME CryptoTests COMMAND test_crypto)

add_executable(test_crypto_adversarial test_crypto_adversarial.cpp)
target_link_libraries(test_crypto_adversarial PRIVATE kasturisundari_core)
add_test(NAME CryptoAdversarialTests COMMAND test_crypto_adversarial)

add_executable(test_blockchain test_blockchain.cpp)
target_link_libraries(test_blockchain PRIVATE kasturisundari_core)
add_test(NAME BlockchainTests COMMAND test_blockchain)

add_executable(test_transactions test_transactions.cpp)
target_link_libraries(test_transactions PRIVATE kasturisundari_core)
add_test(NAME TransactionTests COMMAND test_transactions)

add_executable(test_poc test_poc.cpp)
target_link_libraries(test_poc PRIVATE kasturisundari_core)
add_test(NAME PoCTests COMMAND test_poc)

add_executable(test_vm test_vm.cpp)
target_link_libraries(test_vm PRIVATE kasturisundari_core)
add_test(NAME VMTests COMMAND test_vm)

add_executable(test_attestation test_attestation.cpp)
target_link_libraries(test_attestation PRIVATE kasturisundari_core)
add_test(NAME AttestationTests COMMAND test_attestation)

add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE kasturisundari_core)
add_test(NAME CoreTests COMMAND test_core)

add_executable(test_state test_state.cpp)
target_link_libraries(test_state PRIVATE kasturisundari_core)
add_test(NAME StateTests COMMAND test_state)

add_executable(test_mempool test_mempool.cpp)
target_link_libraries(test_mempool PRIVATE kasturisundari_core)
add_test(NAME MempoolTests COMMAND test_mempool)

add_executable(test_pq test_pq.cpp)
target_link_libraries(test_pq PRIVATE kasturisundari_core)
add_test(NAME PQTests COMMAND test_pq)

add_executable(test_net test_net.cpp)
target_link_libraries(test_net PRIVATE kasturisundari_core)
add_test(NAME NetTests COMMAND test_net)

add_executable(test_rpc test_rpc.cpp)
target_link_libraries(test_rpc PRIVATE kasturisundari_core)
add_test(NAME RpcTests COMMAND test_rpc)

add_executable(test_vedic test_vedic.cpp)
target_link_libraries(test_vedic PRIVATE kasturisundari_core)
add_test(NAME VedicTests COMMAND test_vedic)

add_executable(test_agni_vm test_agni_vm.cpp)
target_link_libraries(test_agni_vm PRIVATE kasturisundari_core)
add_test(NAME AgniVMTests COMMAND test_agni_vm)

add_executable(test_phase10 test_phase10.cpp)
target_link_libraries(test_phase10 PRIVATE kasturisundari_core)
add_test(NAME Phase10Tests COMMAND test_phase10)

add_executable(test_akshara test_akshara.cpp)
target_link_libraries(test_akshara PRIVATE kasturisundari_core)
add_test(NAME AksharaEngineTests COMMAND test_akshara)

add_executable(test_dharma_ordering test_dharma_ordering.cpp)
target_link_libraries(test_dharma_ordering PRIVATE kasturisundari_core)
add_test(NAME DharmaOrderingTests COMMAND test_dharma_ordering)

add_executable(test_succinct_pruning test_succinct_pruning.cpp)
target_link_libraries(test_succinct_pruning PRIVATE kasturisundari_core)
add_test(NAME SuccinctPruningTests COMMAND test_succinct_pruning)

add_executable(test_mesh_lora test_mesh_lora.cpp)
target_link_libraries(test_mesh_lora PRIVATE kasturisundari_core)
add_test(NAME MeshLoraTests COMMAND test_mesh_lora)

add_executable(test_aggkit_pol_restaking test_aggkit_pol_restaking.cpp)
target_link_libraries(test_aggkit_pol_restaking PRIVATE kasturisundari_core)
add_test(NAME AggKitPolRestakingTests COMMAND test_aggkit_pol_restaking)

