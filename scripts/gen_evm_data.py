#!/usr/bin/env python3
import json
import hashlib

# Load 18 chapter attestations to build C++ log array
with open('/home/kali/Desktop/yugala/satya-explorer/src/data/chapters_18_attestation.json', 'r', encoding='utf-8') as f:
    chapters = json.load(f)

FUNCTION_SELECTOR = "f93ebfa1"
MASTER_ROOT = "60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"
EVENT_TOPIC0 = "0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b"

logs = []
tx_map = {}

for ch in chapters:
    ch_hash = ch['sha256Hash']
    tx_hash = ch['anchorTxHash']
    block_num_hex = hex(ch['anchorBlock'])
    
    title_str = f"Chapter {ch['chapter']}: {ch['title']}"
    title_bytes = title_str.encode('utf-8')
    title_hex = title_bytes.hex()
    padded_len = ((len(title_bytes) + 31) // 32) * 32
    title_hex_padded = title_hex.ljust(padded_len * 2, '0')
    
    # ABI calldata
    calldata = "0x" + FUNCTION_SELECTOR + ch_hash.replace("0x","").zfill(64) + "0000000000000000000000000000000000000000000000000000000000000060" + MASTER_ROOT.zfill(64) + hex(len(title_bytes))[2:].zfill(64) + title_hex_padded

    # Event Data: offset 0x20 + string length + string bytes
    event_data = "0x0000000000000000000000000000000000000000000000000000000000000020" + hex(len(title_bytes))[2:].zfill(64) + title_hex_padded

    log_obj = {
        "address": "0x1081080000000000000000000000000000000001",
        "topics": [
            EVENT_TOPIC0,
            ch_hash,
            "0x" + MASTER_ROOT
        ],
        "data": event_data,
        "blockNumber": block_num_hex,
        "transactionHash": tx_hash,
        "transactionIndex": "0x0",
        "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
        "logIndex": hex(ch['chapter'] - 1)
    }
    logs.append(log_obj)

    tx_map[tx_hash] = {
        "hash": tx_hash,
        "nonce": "0x1",
        "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
        "blockNumber": block_num_hex,
        "transactionIndex": "0x0",
        "from": "0x1081080000000000000000000000000000000001",
        "to": "0x1081080000000000000000000000000000000001",
        "value": "0x0",
        "gasPrice": "0x3b9aca00",
        "gas": "0x186a0",
        "input": calldata,
        "log": log_obj
    }

# Also add Master TxHash
master_tx = "0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6"
tx_map[master_tx] = {
    "hash": master_tx,
    "nonce": "0x1",
    "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
    "blockNumber": "0x6c",
    "transactionIndex": "0x0",
    "from": "0x1081080000000000000000000000000000000001",
    "to": "0x1081080000000000000000000000000000000001",
    "value": "0x0",
    "gasPrice": "0x3b9aca00",
    "gas": "0x186a0",
    "input": "0x" + FUNCTION_SELECTOR + "6682b14730dc5404ac8c45e04e0d7cbd6b06e59bdc0f65e9bdb07a752dc8a33a" + "0000000000000000000000000000000000000000000000000000000000000060" + MASTER_ROOT + "0000000000000000000000000000000000000000000000000000000000000015736872696d61645f62686167617661645f676974610000000000000000000000",
    "log": logs[0]
}

print(f"[✓] Formatted {len(logs)} logs and {len(tx_map)} EVM transactions.")

with open('/home/kali/Desktop/yugala/scripts/evm_logs.json', 'w') as f:
    json.dump(logs, f)

with open('/home/kali/Desktop/yugala/scripts/evm_txs.json', 'w') as f:
    json.dump(tx_map, f)
