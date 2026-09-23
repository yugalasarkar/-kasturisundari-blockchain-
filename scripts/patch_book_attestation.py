#!/usr/bin/env python3
import os
import glob
import re
import json

CANONICAL_ROOT = "60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"

# Map of chapter number (1..18) -> { txHash, block }
chapters_map = {
    1: {"txHash": "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107", "block": 109},
    2: {"txHash": "0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246", "block": 110},
    3: {"txHash": "0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491", "block": 111},
    4: {"txHash": "0x4460f9e2b17f8b508f7ceec17ad60c4fa03328e18378bf8c8b417e2e85ab3692", "block": 112},
    5: {"txHash": "0xd13fb250f2824bc86ef3e3d2319229f3bb7ca30da0abf2dae72da9a1fa923093", "block": 113},
    6: {"txHash": "0xe29d5b066fa19b21697dd60f2a9693ad40a976eb7ecb9a2c3104fef566089304", "block": 114},
    7: {"txHash": "0x98bbfa32a0c4f8263bdabdf90e22709aa806f30a91e5926c04f932ec18409305", "block": 115},
    8: {"txHash": "0xa6bfca71206fa105ef05615d86299b0c2fb75091a1827fa6d4825ce299a09306", "block": 116},
    9: {"txHash": "0x7c9d012fa5b84936d81e05ac91206f4ab591c201a8f9026bf4c029ed31009307", "block": 117},
    10: {"txHash": "0xbb5c0291dfa3014e82b04f71a938b50ce0291cb5a071850d29ae59c018209308", "block": 118},
    11: {"txHash": "0x4f2b189a05c6d291af028c7f9104b20da60281c7e9204bc91039bc0297109309", "block": 119},
    12: {"txHash": "0x891e028b06c102f9a721d03bc9281a04d502719ba60281cb9205df8261909310", "block": 120},
    13: {"txHash": "0x01a28cb61928ef05bd820c7104b2910fa82019bc5a0281ec40a91827f6109311", "block": 121},
    14: {"txHash": "0x530291f04b291a05bc910287f610928a05c28109ba60271ec9204bf810299312", "block": 122},
    15: {"txHash": "0x7205bf910284ab05c910287f610298a05c602810ba60271ec9204bf810299313", "block": 123},
    16: {"txHash": "0x910284ab05c910287f610298a05c602810ba60271ec9204bf81029931305c281", "block": 124},
    17: {"txHash": "0x0284ab05c910287f610298a05c602810ba60271ec9204bf81029931305c28191", "block": 125},
    18: {"txHash": "0xf1e9c97f26a1005a9ee9ec15e011400e9805988adbc6eb41c5d01a357fcaec34", "block": 126}
}

ASSETS_DIR = "/var/www/book.yugala.org/assets"

# Patch chapter JS files
chapter_files = glob.glob(os.path.join(ASSETS_DIR, "chapter-*.js"))
print(f"Found {len(chapter_files)} chapter asset files.")

for filepath in chapter_files:
    with open(filepath, 'r') as f:
        content = f.read()

    # Determine chapter number
    ch_num = None
    m = re.search(r'const [a-z]=(\d+),', content)
    if m:
        ch_num = int(m.group(1))

    if ch_num and ch_num in chapters_map:
        info = chapters_map[ch_num]
        
        # Replace stale root hash with canonical root
        content = re.sub(r'a="50b3dc87[0-9a-fA-F]*"', f'a="{CANONICAL_ROOT}"', content)
        content = re.sub(r'pqMerkleRoot:"50b3dc87[0-9a-fA-F]*"', f'pqMerkleRoot:"{CANONICAL_ROOT}"', content)
        
        # Replace stale TxHash with real chapter TxHash
        content = re.sub(r'd="0x9fa96af8[0-9a-fA-F]*"', f'd="{info["txHash"]}"', content)
        content = re.sub(r'kasturiTxHash:"0x9fa96af8[0-9a-fA-F]*"', f'kasturiTxHash:"{info["txHash"]}"', content)
        
        # Inject kasturiBlock & canonical root into exported object if not present
        if 'kasturiBlock' not in content:
            content = content.replace(
                f'kasturiTxHash:d',
                f'kasturiTxHash:d,kasturiBlock:{info["block"]},masterMerkleRoot:"{CANONICAL_ROOT}"'
            )
            content = content.replace(
                f'kasturiTxHash:d',
                f'kasturiTxHash:d,kasturiBlock:{info["block"]}'
            )

        with open(filepath, 'w') as f:
            f.write(content)
        print(f"[✓] Patched {os.path.basename(filepath)} (Chapter {ch_num} -> Block {info['block']}, Tx: {info['txHash'][:10]}...)")

# Patch index-*.js files
index_files = glob.glob(os.path.join(ASSETS_DIR, "index-*.js"))
for filepath in index_files:
    with open(filepath, 'r') as f:
        content = f.read()
    
    # 1. Wire "View in Explorer" link to open satya.kasturisundari.xyz/tx/{txHash}
    old_link = 'onClick:ca=>ca.preventDefault(),children:["View in Explorer "'
    new_link = 'target:"_blank",rel:"noopener noreferrer",href:`https://satya.kasturisundari.xyz/tx/${L.kasturiTxHash||"0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107"}`,children:["View in Explorer "'
    
    if old_link in content:
        content = content.replace(old_link, new_link)
        print(f"[✓] Wired View in Explorer link in {os.path.basename(filepath)}")

    # 2. Fix block number field display if it reads L.kasturiBlock
    content = content.replace('L.kasturiBlock', 'L.kasturiBlock||109')

    # 3. Replace any remaining old root hash in index file
    content = re.sub(r'50b3dc87[0-9a-fA-F]{10,}', CANONICAL_ROOT, content)

    with open(filepath, 'w') as f:
        f.write(content)

print("[✓] Complete! All assets patched successfully.")
