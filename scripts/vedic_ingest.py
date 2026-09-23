#!/usr/bin/env python3
import json
import os
import glob

# Paths
SHASTRA_DIR = "/home/kali/Desktop/sutra-lang/shastra"
OUTPUT_DIR = "/home/kali/Desktop/yugala/dataset_out"

os.makedirs(OUTPUT_DIR, exist_ok=True)

def process_rigveda():
    print("Processing Rigveda...")
    for i in range(1, 11):
        path = f"{SHASTRA_DIR}/rigveda/book_{i}.json"
        if not os.path.exists(path): continue
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        out_path = f"{OUTPUT_DIR}/rigveda_book_{i}.kasturi"
        with open(out_path, 'w', encoding='utf-8') as out:
            out.write(f"TITLE: Rigveda Book {i}\n")
            out.write(f"CATEGORY: 1\n") # VEDA = 1
            out.write(f"LANGUAGE: Sanskrit\n")
            for v in data:
                ch = int(str(v.get("chapter_number", "1")).split('.')[0])
                v_num = int(v.get("verse_number", 1))
                text = v.get("text", "").strip().replace('\n', ' ')
                if text:
                    out.write(f"VERSE: {ch} {v_num} {text}\n")

def process_atharvaveda():
    print("Processing Atharvaveda...")
    for i in range(1, 21):
        path = f"{SHASTRA_DIR}/atharvaveda/book_{i}.json"
        if not os.path.exists(path): continue
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        out_path = f"{OUTPUT_DIR}/atharvaveda_book_{i}.kasturi"
        with open(out_path, 'w', encoding='utf-8') as out:
            out.write(f"TITLE: Atharvaveda Book {i}\n")
            out.write(f"CATEGORY: 1\n") 
            out.write(f"LANGUAGE: Sanskrit\n")
            for v in data:
                ch = int(str(v.get("chapter_number", "1")).split('.')[0])
                v_num = int(v.get("verse_number", 1))
                text = v.get("text", "").strip().replace('\n', ' ')
                if text:
                    out.write(f"VERSE: {ch} {v_num} {text}\n")

def process_yajurveda():
    print("Processing Yajurveda (Shukla)...")
    path = f"{SHASTRA_DIR}/yajurveda/yajurveda.json"
    if os.path.exists(path):
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        out_path = f"{OUTPUT_DIR}/yajurveda.kasturi"
        with open(out_path, 'w', encoding='utf-8') as out:
            out.write(f"TITLE: Shukla Yajurveda\n")
            out.write(f"CATEGORY: 1\n") 
            out.write(f"LANGUAGE: Sanskrit\n")
            for v in data:
                ch = int(str(v.get("chapter_number", "1")).split('.')[0])
                v_num = int(v.get("verse_number", 1))
                text = v.get("text", "").strip().replace('\n', ' ')
                if text:
                    out.write(f"VERSE: {ch} {v_num} {text}\n")

def process_krishnayajurveda():
    print("Processing Krishna Yajurveda...")
    path = f"{SHASTRA_DIR}/krishnayajurveda/krishnayajurveda.json"
    if os.path.exists(path):
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        out_path = f"{OUTPUT_DIR}/krishnayajurveda.kasturi"
        with open(out_path, 'w', encoding='utf-8') as out:
            out.write(f"TITLE: Krishna Yajurveda\n")
            out.write(f"CATEGORY: 1\n") 
            out.write(f"LANGUAGE: Sanskrit\n")
            for v in data:
                ch = int(str(v.get("chapter_number", "1")).split('.')[0])
                v_num = int(v.get("verse_number", 1))
                text = v.get("text", "").strip().replace('\n', ' ')
                if text:
                    out.write(f"VERSE: {ch} {v_num} {text}\n")

def process_samaveda():
    print("Processing Samaveda...")
    path = f"{SHASTRA_DIR}/samaveda/samaveda.json"
    if os.path.exists(path):
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        out_path = f"{OUTPUT_DIR}/samaveda.kasturi"
        with open(out_path, 'w', encoding='utf-8') as out:
            out.write(f"TITLE: Samaveda\n")
            out.write(f"CATEGORY: 1\n") 
            out.write(f"LANGUAGE: Sanskrit\n")
            for v in data:
                ch = int(str(v.get("chapter_number", "1")).split('.')[0])
                v_num = int(v.get("verse_number", 1))
                text = v.get("text", "").strip().replace('\n', ' ')
                if text:
                    out.write(f"VERSE: {ch} {v_num} {text}\n")

def process_gita():
    print("Processing Bhagavad Gita...")
    path = f"{SHASTRA_DIR}/gita/gita.json"
    if os.path.exists(path):
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        out_path = f"{OUTPUT_DIR}/gita.kasturi"
        with open(out_path, 'w', encoding='utf-8') as out:
            out.write(f"TITLE: Bhagavad Gita\n")
            out.write(f"CATEGORY: 4\n") # ITIHASA = 4
            out.write(f"LANGUAGE: Sanskrit\n")
            for v in data:
                ch = int(v.get("chapter_number", 1))
                v_num = int(v.get("verse_number", 1))
                text = v.get("text", "").strip().replace('\n', ' ')
                if text:
                    out.write(f"VERSE: {ch} {v_num} {text}\n")

if __name__ == "__main__":
    process_rigveda()
    process_atharvaveda()
    process_yajurveda()
    process_krishnayajurveda()
    process_samaveda()
    process_gita()
    print(f"Validation and parsing complete. Output saved to {OUTPUT_DIR}")
