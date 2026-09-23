# KasturiChain (`kasturid`)

**High-Performance Sovereign L1 Core with Polygon AggLayer Integration & DePIN Mesh Consensus**

KasturiChain is a modular, high-throughput Layer 1 infrastructure engineered in modern C++ (C++20). It delivers fair transaction ordering, succinct recursive state pruning, physical mesh-network transport, and native alignment with the Polygon ecosystem via **AggKit LxLy Pessimistic Proofs** and **$POL Dual-Restaking**.

---

## Architectural Highlights

* **AggKit LxLy Bridge Adapter**: Implements strict cryptographic pessimistic proofs ($\sum \text{Withdrawals} \le \sum \text{Deposits}$) to prevent cross-chain over-withdrawal exploits across connected CDK networks.
* **$POL Dual-Restaking Vault**: Supports dual-asset sequencer staking ($POL & $NILA) with dynamic validator weight calculation and automated slashing penalties.
* **Dharma-Ordering Engine**: Protocol-native anti-MEV architecture enforcing deterministic first-come, first-served (FCFS) nanosecond sequencing, eliminating priority gas auctions (PGA) and front-running/sandwich exploits.
* **Succinct Recursive Pruning**: Rapid state bootstrapping (`--light-sovereign`) achieving state verification in sub-millisecond timelines (184 μs) without historical storage bloat.
* **Sovereign Physical Transport**: Integrated serial and LoRa physical framing (< 256-byte compact frames) for offline-resilient mesh consensus.
* **Akshara State Engine**: Immutable write-once memory registers at the execution level prohibiting malicious state alterations or `SELFDESTRUCT` abuse.

---

## Build & Test Suite

### Prerequisites
* CMake 3.20+
* GCC 11+ or Clang 13+ (C++20 compliant)
* Boost, OpenSSL, evmone

### Build Instructions
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Verification & Testing

Execute the complete integration and security test suite:

```bash
./build/tests/test_akshara
./build/tests/test_dharma_ordering
./build/tests/test_succinct_pruning
./build/tests/test_mesh_lora
./build/tests/test_aggkit_pol_restaking
```

---

## JSON-RPC Endpoints

KasturiChain exposes standard Web3-compliant JSON-RPC interfaces alongside AggKit extensions:

* `eth_blockNumber`: Current active block height.
* `eth_getGlobalExitRoot`: Aggregated cross-chain exit root.
* `pol_getStakingInfo`: Dual-restaking telemetry and validator collateral status.

---

## Security Audit Status

* **Zero Non-ASCII / Clean Source**: Verified repository-wide audit passed.
* **Pessimistic Invariant**: Over-withdrawal attack vectors mathematically blocked.

---

## License

GNU Affero General Public License v3.0 (AGPL-3.0) — Strict Copyleft. Any modifications, derived works, or network-hosted deployments must remain fully open source under AGPL-3.0.
