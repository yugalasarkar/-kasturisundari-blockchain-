// Kasturisundari Chain — Panini Engine Implementation

#include "kasturisundari/attestation/panini_engine.h"
#include <cstdint>
#include <string>

namespace kasturisundari {
namespace vedic {

// Simplified checks based on Unicode block for Devanagari (0x0900 - 0x097F)

bool PaniniEngine::is_svara(uint32_t codepoint) {
    // Independent vowels 0x0904 to 0x0914
    // Dependent vowel signs 0x093E to 0x094C
    return (codepoint >= 0x0904 && codepoint <= 0x0914) ||
           (codepoint >= 0x093E && codepoint <= 0x094C);
}

bool PaniniEngine::is_vyanjana(uint32_t codepoint) {
    // Consonants 0x0915 to 0x0939
    return (codepoint >= 0x0915 && codepoint <= 0x0939);
}

bool PaniniEngine::is_modifier(uint32_t codepoint) {
    // Anusvara (0x0902), Visarga (0x0903), Virama (Halant) (0x094D)
    return codepoint == 0x0902 || codepoint == 0x0903 || codepoint == 0x094D;
}

ValidationResult PaniniEngine::validate_sanskrit_text(const std::string& utf8_text) {
    // Basic zero-dependency UTF-8 decoder and Panini structure validator
    size_t i = 0;
    while (i < utf8_text.length()) {
        uint8_t byte = utf8_text[i];
        uint32_t codepoint = 0;
        size_t char_len = 0;

        if ((byte & 0x80) == 0) {
            codepoint = byte;
            char_len = 1;
        } else if ((byte & 0xE0) == 0xC0) {
            if (i + 1 >= utf8_text.length()) return {false, "Invalid UTF-8 sequence", i};
            codepoint = ((byte & 0x1F) << 6) | (utf8_text[i+1] & 0x3F);
            char_len = 2;
        } else if ((byte & 0xF0) == 0xE0) {
            if (i + 2 >= utf8_text.length()) return {false, "Invalid UTF-8 sequence", i};
            codepoint = ((byte & 0x0F) << 12) | ((utf8_text[i+1] & 0x3F) << 6) | (utf8_text[i+2] & 0x3F);
            char_len = 3;
        } else {
            return {false, "Unsupported UTF-8 encoding or non-Devanagari script", i};
        }

        // We skip whitespace and generic punctuation, but REJECT English/Latin alphabets
        if (codepoint <= 0x007F) {
            if ((codepoint >= 'a' && codepoint <= 'z') || (codepoint >= 'A' && codepoint <= 'Z')) {
                return {false, "Contains Latin/English alphabet characters", i};
            }
            i += char_len;
            continue;
        }

        // Ensure it falls in the Devanagari block or Vedic extensions (0x1CD0 - 0x1CFF)
        bool is_devanagari = (codepoint >= 0x0900 && codepoint <= 0x097F);
        bool is_vedic_ext = (codepoint >= 0x1CD0 && codepoint <= 0x1CFF);

        if (!is_devanagari && !is_vedic_ext) {
            return {false, "Contains non-Sanskrit/Devanagari characters", i};
        }

        i += char_len;
    }

    return {true, "Valid", 0};
}

} // namespace vedic
} // namespace kasturisundari
