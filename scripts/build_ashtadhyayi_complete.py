#!/usr/bin/env python3
"""
Build complete Ashtadhyayi corpus by merging:
- data-master/ (structured JSON: sutras, dhatus, ganapatha, unadi, linganushasanam, shivasutra, paribhasha, vakyapadeeyam, krut)
- data-edit-main/sutraani/ (individual commentary files per sutra: kashika, kaumudi, bhashya, etc.)
"""

import json
import os
from pathlib import Path
from typing import Dict, List, Any, Optional

BASE_DIR = Path("/home/kali/Desktop/yugala")
DATA_MASTER = BASE_DIR / "data-master"
DATA_EDIT = BASE_DIR / "data-edit-main"
OUTPUT_FILE = BASE_DIR / "ashtadhyayi_complete.json"

COMMENTARIES = [
    "kashika",
    "kaumudi",
    "bhashya",
    "laghukaumudi",
    "laghushabdendushekhar",
    "nyaas",
    "padamanjari",
    "praudhamanorama",
    "sarala",
    "sudha",
    "tattvabodhini",
    "vasu_english",
    "balamanorama",
]

PRIMARY_COMMENTARIES = {"kashika", "kaumudi", "bhashya", "laghukaumudi"}


def load_json_file(path: Path) -> Dict:
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)


def load_commentary_file(commentary_dir: Path, sutra_id: str) -> Optional[str]:
    """Load commentary text for a specific sutra ID."""
    file_path = commentary_dir / f"{sutra_id}.txt"
    if file_path.exists():
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read().strip()
    return None


def parse_sutra_type(type_str: str) -> tuple:
    """Parse type field like 'S$वृद्धिसंज्ञा$' or 'P$...$' or 'V$$'."""
    if not type_str:
        return ("Unknown", "")
    parts = type_str.split('$')
    type_code = parts[0] if parts else "Unknown"
    type_name = parts[1] if len(parts) > 1 else ""
    
    type_map = {
        "S": "Samjñā (संज्ञा)",
        "P": "Paribhāṣā (परिभाषा)",
        "V": "Vidhi (विधि)",
        "N": "Niyama (नियम)",
        "A": "Atideśa (अतिदेश)",
        "D": "Adhikāra (अधिकार)",
    }
    return (type_map.get(type_code, type_code), type_name)


def parse_anuvritti(an_str: str) -> List[Dict]:
    """Parse anuvritti field like 'वृद्धिः$11001##गुणः$11002'."""
    if not an_str:
        return []
    result = []
    for item in an_str.split('##'):
        if '$' in item:
            word, ref = item.split('$', 1)
            result.append({"word": word, "sutra_ref": ref})
    return result


def parse_padaccheda(pc_str: str) -> str:
    """Parse padaccheda field with $S$ markers."""
    if not pc_str:
        return ""
    return pc_str.replace('$S$', ' ').replace('$', ' ').replace('#', ' ').strip()


def build_sutra_id(adhyaya: str, pada: str, number: str) -> str:
    """Build sutra ID like 11001 from components."""
    return f"{int(adhyaya):01d}{int(pada):01d}{int(number):03d}"


def load_sutras() -> List[Dict]:
    """Load and process all sutras with commentaries."""
    print("Loading sutras from data-master...")
    sutra_data = load_json_file(DATA_MASTER / "sutraani" / "data.txt")
    sutras_raw = sutra_data.get("data", [])
    
    # Build commentary lookup
    commentary_dirs = {}
    for comm in COMMENTARIES:
        comm_path = DATA_EDIT / "sutraani" / comm
        if comm_path.exists():
            commentary_dirs[comm] = comm_path
    
    print(f"Found {len(commentary_dirs)} commentary directories")
    
    sutras = []
    for s in sutras_raw:
        sutra_id = s.get("i", "")
        
        # Load all commentaries for this sutra
        commentaries = {}
        for comm_name, comm_dir in commentary_dirs.items():
            text = load_commentary_file(comm_dir, sutra_id)
            if text:
                commentaries[comm_name] = text
        
        sutra_type, type_name = parse_sutra_type(s.get("type", ""))
        
        sutra = {
            "id": sutra_id,
            "number": f"{s.get('a')}.{s.get('p')}.{s.get('n')}",
            "adhyaya": int(s.get("a", 0)),
            "pada": int(s.get("p", 0)),
            "sutra_number": int(s.get("n", 0)),
            "text": s.get("s", ""),
            "iast": s.get("e", ""),
            "type": sutra_type,
            "type_name": type_name,
            "padachheda": parse_padaccheda(s.get("pc", "")),
            "anuvritti": parse_anuvritti(s.get("an", "")),
            "adhikara": s.get("ad", ""),
            "summary": s.get("ss", ""),
            "commentaries": commentaries,
            "refs": {
                "kashika_sutra": s.get("skn"),
                "laghukaumudi_sutra": s.get("lskn"),
                "mahabhashya_sutra": s.get("mskn"),
                "siddhantakaumudi_sutra": s.get("sskn"),
                "praudhamanorama_sutra": s.get("plskn"),
                "laghushabdendushekhar_sutra": s.get("lpn"),
            }
        }
        sutras.append(sutra)
    
    print(f"Processed {len(sutras)} sutras")
    return sutras


def load_dhatus() -> List[Dict]:
    """Load dhatus with all 10 lakara forms."""
    print("Loading dhatus...")
    dhatu_data = load_json_file(DATA_MASTER / "dhatu" / "data.txt")
    dhatus_raw = dhatu_data.get("data", [])
    
    gana_names = {
        "1": "भ्वादिगणः (Bhvādi)",
        "2": "अदादिगणः (Adādi)",
        "3": "जुहोत्यादिगणः (Juhotyādi)",
        "4": "दिवादिगणः (Divādi)",
        "5": "स्वादिगणः (Svādi)",
        "6": "तुदादिगणः (Tudādi)",
        "7": "रुधादिगणः (Rudhādi)",
        "8": "तनादिगणः (Tanādi)",
        "9": "क्र्यादिगणः (Kryādi)",
        "10": "चुरादिगणः (Curādi)",
    }
    
    pada_names = {
        "P": "परस्मैपदी (Parasmaipada)",
        "A": "आत्मनेपदी (Ātmanepada)",
        "U": "उभयपदी (Ubhayapada)",
    }
    
    it_names = {
        "S": "सेट् (Seṭ)",
        "N": "अनिट् (Aniṭ)",
        "V": "वेट् (Veṭ)",
    }
    
    dhatus = []
    for d in dhatus_raw:
        dhatu = {
            "code": d.get("baseindex", ""),
            "root": d.get("dhatu", ""),
            "aupadeshik": d.get("aupadeshik", ""),
            "gana_num": d.get("gana", ""),
            "gana": gana_names.get(str(d.get("gana", "")), ""),
            "pada": pada_names.get(d.get("pada", ""), d.get("pada", "")),
            "it_status": it_names.get(d.get("settva", ""), d.get("settva", "")),
            "meaning_sanskrit": d.get("artha", ""),
            "meaning_english": d.get("artha_english", ""),
            "meaning_hindi": d.get("artha_hindi", ""),
            "tags": d.get("tags", ""),
            "forms": {},  # Will be populated from vidyut files if needed
        }
        dhatus.append(dhatu)
    
    print(f"Processed {len(dhatus)} dhatus")
    return dhatus


def load_ganapatha() -> List[Dict]:
    """Load ganapatha (word groups)."""
    print("Loading ganapatha...")
    data = load_json_file(DATA_MASTER / "ganapath" / "data.txt")
    ganas = data.get("data", [])
    
    result = []
    for g in ganas:
        result.append({
            "index": g.get("ind", 0),
            "name": g.get("name", ""),
            "sutra_ref": g.get("sutra", ""),
            "vartika": g.get("vartika", ""),
            "words": g.get("words", ""),
            "type": g.get("type", ""),
        })
    
    print(f"Processed {len(result)} gana entries")
    return result


def load_unadi() -> List[Dict]:
    """Load unadi sutras."""
    print("Loading unadi sutras...")
    data = load_json_file(DATA_MASTER / "unaadi" / "data.txt")
    unadi_raw = data.get("data", [])
    
    result = []
    for u in unadi_raw:
        result.append({
            "index": u.get("i", ""),
            "sutra": u.get("sutra", ""),
            "pratyaya": u.get("pratyay", ""),
            "explanation": u.get("sk", ""),
        })
    
    print(f"Processed {len(result)} unadi sutras")
    return result


def load_linganushasanam() -> List[Dict]:
    """Load linganushasanam (gender rules)."""
    print("Loading linganushasanam...")
    data = load_json_file(DATA_MASTER / "linganushasanam" / "data.txt")
    linga_raw = data.get("data", [])
    
    result = []
    for l in linga_raw:
        result.append({
            "id": l.get("id", ""),
            "adhikara": l.get("adhikaar", ""),
            "sutra": l.get("sutra", ""),
            "explanation": l.get("sk", ""),
        })
    
    print(f"Processed {len(result)} linganushasanam entries")
    return result


def load_shivasutras() -> List[Dict]:
    """Load Maheshwara sutras with kashika and vyakhya."""
    print("Loading Maheshwara sutras...")
    data = load_json_file(DATA_MASTER / "shivasutra" / "data.txt")
    shiva_raw = data.get("data", [])
    
    result = []
    for s in shiva_raw:
        result.append({
            "id": s.get("id", ""),
            "sutra": s.get("sutra", ""),
            "kashika": s.get("kashika", ""),
            "vyakhya": s.get("vyakhya", ""),
        })
    
    print(f"Processed {len(result)} Maheshwara sutras")
    return result


def load_paribhashas() -> List[Dict]:
    """Load paribhashas (meta-rules)."""
    print("Loading paribhashas...")
    data = load_json_file(DATA_MASTER / "paribhashendushekhar" / "data.txt")
    paribhasha_raw = data.get("data", [])
    
    result = []
    for p in paribhasha_raw:
        result.append({
            "id": p.get("id", ""),
            "sutra": p.get("sutra", ""),
            "kashika": p.get("kashika", ""),
            "vyakhya": p.get("vyakhya", ""),
        })
    
    print(f"Processed {len(result)} paribhashas")
    return result


def load_vakyapadeeyam() -> List[Dict]:
    """Load Vakyapadeeyam verses."""
    print("Loading Vakyapadeeyam...")
    data = load_json_file(DATA_MASTER / "vakyapadeeyam" / "data.txt")
    vakya_raw = data.get("data", [])
    
    result = []
    for v in vakya_raw:
        result.append({
            "num": v.get("num", ""),
            "id": v.get("id", ""),
            "kanda": v.get("kanda", ""),
            "samuddesh": v.get("samuddesh", ""),
            "adhikara": v.get("adhikaar", ""),
            "text": v.get("text", ""),
            "meaning": v.get("artha", ""),
        })
    
    print(f"Processed {len(result)} Vakyapadeeyam verses")
    return result


def load_krut() -> Dict:
    """Load krut data (groups, prakruti, pratyaya)."""
    print("Loading krut data...")
    krut_data = {}
    
    for fname in ["groups.txt", "prakruti.txt", "pratyay.txt"]:
        path = DATA_MASTER / "krut" / fname
        if path.exists():
            data = load_json_file(path)
            krut_data[fname.replace(".txt", "")] = data
    
    return krut_data


def load_pratyaharas() -> Dict:
    """Load pratyahara data."""
    print("Loading pratyaharas...")
    data = load_json_file(DATA_MASTER / "pratyahara" / "data.txt")
    return data


def build_pratyahara_map(shivasutras: List[Dict]) -> Dict[str, List[str]]:
    """Build pratyahara map from Maheshwara sutras."""
    # The 14 Maheshwara sutras in order
    sutra_letters = []
    for s in shivasutras:
        sutra_text = s.get("sutra", "")
        # Extract letters (remove it-markers at end)
        letters = []
        for ch in sutra_text:
            if ch not in 'क्ङचञटणतमपयशषसहल्ङ्ञ':
                letters.append(ch)
        sutra_letters.append(letters)
    
    # Build all pratyaharas
    pratyaharas = {}
    # This is a simplified version - the full pratyahara list is in data-master/pratyahara/data.txt
    return pratyaharas


def main():
    print("=" * 60)
    print("BUILDING COMPLETE ASHTADHYAYI CORPUS")
    print("=" * 60)
    
    # Load all components
    sutras = load_sutras()
    dhatus = load_dhatus()
    ganapatha = load_ganapatha()
    unadi = load_unadi()
    linganushasanam = load_linganushasanam()
    shivasutras = load_shivasutras()
    paribhashas = load_paribhashas()
    vakyapadeeyam = load_vakyapadeeyam()
    krut = load_krut()
    pratyahara_data = load_pratyaharas()
    
    # Build pratyahara map from shivasutras
    pratyahara_map = build_pratyahara_map(shivasutras)
    
    # Count commentaries
    commentary_counts = {}
    for s in sutras:
        for comm in s["commentaries"]:
            commentary_counts[comm] = commentary_counts.get(comm, 0) + 1
    
    print("\nCommentary coverage:")
    for comm, count in sorted(commentary_counts.items()):
        pct = (count / len(sutras)) * 100
        marker = " ★" if comm in PRIMARY_COMMENTARIES else ""
        print(f"  {comm}: {count}/{len(sutras)} ({pct:.1f}%){marker}")
    
    # Build final structure
    corpus = {
        "metadata": {
            "version": "1.0",
            "title": "अष्टाध्यायी पूर्णकोशः — Complete Pāṇinian Corpus",
            "source": "data-master (structured) + data-edit-main (commentaries)",
            "tradition": "Pāṇinian Corpus (Pañcāṅga Vyākaraṇa)",
            "script": "Devanāgarī",
            "stats": {
                "total_sutras": len(sutras),
                "total_dhatus": len(dhatus),
                "total_ganas": len(ganapatha),
                "total_unadi": len(unadi),
                "total_linganushasanam": len(linganushasanam),
                "total_shivasutras": len(shivasutras),
                "total_paribhashas": len(paribhashas),
                "total_vakyapadeeyam": len(vakyapadeeyam),
            },
            "commentaries_available": list(commentary_counts.keys()),
            "primary_commentaries": list(PRIMARY_COMMENTARIES),
        },
        "paniniya_pancanga": {
            "sutrapatha": sutras,
            "dhatupatha": dhatus,
            "ganapatha": ganapatha,
            "unadipatha": unadi,
            "linganushasanam": linganushasanam,
        },
        "foundational_tools": {
            "maheshwara_sutras": shivasutras,
            "vartikapatha": [],  # Extracted from sutra commentaries
            "paribhashapatha": paribhashas,
        },
        "related_texts": {
            "vakyapadeeyam": vakyapadeeyam,
            "krut": krut,
            "pratyaharas": pratyahara_data,
        },
        "computational": {
            "pratyahara_map": pratyahara_map,
            "sutra_index": {s["id"]: i for i, s in enumerate(sutras)},
            "dhatu_index": {d["code"]: i for i, d in enumerate(dhatus)},
            "gana_index": {g["name"]: i for i, g in enumerate(ganapatha)},
        }
    }
    
    # Write output
    print(f"\nWriting to {OUTPUT_FILE}...")
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        json.dump(corpus, f, ensure_ascii=False, separators=(',', ':'))
    
    size_mb = OUTPUT_FILE.stat().st_size / (1024 * 1024)
    print(f"Done! Output: {OUTPUT_FILE} ({size_mb:.1f} MB)")
    
    # Print summary
    print("\n" + "=" * 60)
    print("CORPUS SUMMARY")
    print("=" * 60)
    print(f"Sūtras:        {len(sutras):>6} (8 Adhyāyas × 4 Pādas)")
    print(f"Dhātus:        {len(dhatus):>6} (10 Gaṇas)")
    print(f"Gaṇas:         {len(ganapatha):>6}")
    print(f"Unādi Sūtras:  {len(unadi):>6}")
    print(f"Liṅgānuśāsana: {len(linganushasanam):>6}")
    print(f"Māheśvara:     {len(shivasutras):>6}")
    print(f"Paribhāṣās:    {len(paribhashas):>6}")
    print(f"Vākyapadīyam:  {len(vakyapadeeyam):>6}")
    print("-" * 60)
    print(f"Primary commentaries per sūtra: Kāśikā, Kaumudī, Mahābhāṣya, Laghu Kaumudī")
    print(f"Additional commentaries: {len(COMMENTARIES) - len(PRIMARY_COMMENTARIES)} more available on-demand")
    print("=" * 60)


if __name__ == "__main__":
    main()