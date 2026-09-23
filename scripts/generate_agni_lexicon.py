#!/usr/bin/env python3
# Kasturisundari Chain — Agni Bhasha Lexicon Generator (Optimized)

import os

OUTPUT_DIR = "include/kasturisundari/vm"
OUTPUT_FILE = os.path.join(OUTPUT_DIR, "agni_lexicon.h")

CORE_MAPPINGS = {
    "वृद्धिरादैच्": "OP_ALLOC",
    "अकः सवर्णे दीर्घः": "OP_ADD",
    "इको यणचि": "OP_SUB",
    "तस्य लोपः": "OP_DELETE",
    "स्थानिवदादेशोऽनल्विधौ": "OP_JUMP",
    "अदर्शनं लोपः": "OP_RETURN",
    "हलन्त्यम्": "OP_END",
    "दा": "FUNC_TRANSFER",
    "कृ": "FUNC_EXECUTE",
    "ज्ञा": "FUNC_READ_STATE",
    "रक्ष्": "FUNC_WRITE_STATE"
}

def generate_devanagari_mock(prefix, count, offset):
    words = []
    base_char = 0x0915
    vowel = 0x093E
    for i in range(count):
        word = chr(base_char + (i % 30)) + chr(vowel + (i % 10)) + chr(base_char + ((i+offset) % 30))
        words.append(word)
    return words

def main():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    dhatus = generate_devanagari_mock("dhatu", 2000, 5)
    sutras = generate_devanagari_mock("sutra", 4000, 15)

    dhatu_index = 0
    sutra_index = 0
    for key, val in CORE_MAPPINGS.items():
        if val.startswith("FUNC"):
            dhatus[dhatu_index] = key
            dhatu_index += 1
        elif val.startswith("OP"):
            sutras[sutra_index] = key
            sutra_index += 1

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write("// Kasturisundari Chain — Agni Bhasha Lexicon\n")
        f.write("// AUTO-GENERATED FILE. DO NOT EDIT.\n")
        f.write("#pragma once\n\n")
        f.write("#include <unordered_map>\n")
        f.write("#include <string>\n\n")
        f.write("namespace kasturisundari {\n")
        f.write("namespace vm {\n\n")
        
        f.write("enum class AgniOpcode {\n")
        f.write("    OP_UNKNOWN = 0,\n")
        for val in CORE_MAPPINGS.values():
            f.write(f"    {val},\n")
        f.write("    OP_MOCK_RULE,\n")
        f.write("    FUNC_MOCK_ROOT\n")
        f.write("};\n\n")

        f.write("struct AgniLexicon {\n")
        f.write("    static AgniOpcode get_sutra(const std::string& word) {\n")
        f.write("        static std::unordered_map<std::string, AgniOpcode> map;\n")
        f.write("        if (map.empty()) {\n")
        
        # Write array initialization which is fast to compile
        f.write("            struct Entry { const char* k; AgniOpcode v; };\n")
        f.write("            static const Entry entries[] = {\n")
        for i, sutra in enumerate(sutras):
            op = CORE_MAPPINGS.get(sutra, "OP_MOCK_RULE")
            f.write(f'                {{"{sutra}", AgniOpcode::{op}}},\n')
        f.write("            };\n")
        f.write("            for (const auto& e : entries) map[e.k] = e.v;\n")
        f.write("        }\n")
        f.write("        auto it = map.find(word);\n")
        f.write("        return it != map.end() ? it->second : AgniOpcode::OP_UNKNOWN;\n")
        f.write("    }\n\n")

        f.write("    static AgniOpcode get_dhatu(const std::string& word) {\n")
        f.write("        static std::unordered_map<std::string, AgniOpcode> map;\n")
        f.write("        if (map.empty()) {\n")
        f.write("            struct Entry { const char* k; AgniOpcode v; };\n")
        f.write("            static const Entry entries[] = {\n")
        for i, dhatu in enumerate(dhatus):
            op = CORE_MAPPINGS.get(dhatu, "FUNC_MOCK_ROOT")
            f.write(f'                {{"{dhatu}", AgniOpcode::{op}}},\n')
        f.write("            };\n")
        f.write("            for (const auto& e : entries) map[e.k] = e.v;\n")
        f.write("        }\n")
        f.write("        auto it = map.find(word);\n")
        f.write("        return it != map.end() ? it->second : AgniOpcode::OP_UNKNOWN;\n")
        f.write("    }\n")
        f.write("};\n\n")

        f.write("} // namespace vm\n")
        f.write("} // namespace kasturisundari\n")

if __name__ == "__main__":
    main()
