#pragma once
#include <cstdint>
#include <optional>
namespace fw::gw::quests {
// Both clocks are sampled by the server. The client supplies neither value.
// Preserve occurrence time when processing a queued kill after its deadline.
inline std::optional<std::uint64_t> event_utc_ms(std::uint64_t occurred_mono_ms,
        std::uint64_t sampled_mono_ms,std::uint64_t sampled_utc_ms){
    if(occurred_mono_ms>sampled_mono_ms)return {};
    const auto age=sampled_mono_ms-occurred_mono_ms;
    if(sampled_utc_ms<age)return {};
    return sampled_utc_ms-age;
}
}
