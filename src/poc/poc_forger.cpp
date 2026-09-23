// Kasturisundari Chain — PoC Forger Implementation

#include "kasturisundari/poc/poc_forger.h"

#include <algorithm>

namespace kasturisundari {
namespace poc {

uint64_t get_poc_score(const std::string& address, const state::StateDB& state) {
    std::string key = "poc_score:" + address;
    auto val = state.get_meta(key);
    if (val.has_value() && !val->empty()) {
        try {
            return std::stoull(val.value());
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

void increment_poc_score(const std::string& address, state::StateDB& state) {
    uint64_t current = get_poc_score(address, state);
    state.put_meta("poc_score:" + address, std::to_string(current + 1));
}

uint64_t compute_forging_interval(uint64_t poc_score) {
    uint64_t bonus = poc_score * SCORE_BONUS_PER_CONTRIBUTION;
    if (bonus >= BASE_FORGING_INTERVAL - MIN_FORGING_INTERVAL) {
        return MIN_FORGING_INTERVAL;
    }
    return BASE_FORGING_INTERVAL - bonus;
}

} // namespace poc
} // namespace kasturisundari
