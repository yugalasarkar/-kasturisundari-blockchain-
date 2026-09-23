// Kasturisundari Chain — Sanskrit Grammar Engine (Panini Validation)
// Validates Sanskrit texts submitted for attestation against basic structural rules.
// Ensures that only mathematically/grammatically valid Sanskrit (Shabda) enters the chain.

#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace kasturisundari {
namespace vedic {

/// Represents a validation result from the Panini Engine.
struct ValidationResult {
    bool is_valid;
    std::string error_message;
    size_t error_position; // Byte index where error occurred
};

class PaniniEngine {
public:
    /// Validates a UTF-8 encoded Devanagari string.
    /// Checks for valid consonant-vowel combinations and structural integrity.
    static ValidationResult validate_sanskrit_text(const std::string& utf8_text);

    /// Checks if a character is a Devanagari consonant (Vyanjana).
    static bool is_vyanjana(uint32_t codepoint);

    /// Checks if a character is a Devanagari vowel (Svara).
    static bool is_svara(uint32_t codepoint);

    /// Checks if a character is a Devanagari modifier (Anusvara, Visarga, Virama, etc).
    static bool is_modifier(uint32_t codepoint);
};

} // namespace vedic
} // namespace kasturisundari
