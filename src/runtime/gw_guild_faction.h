#pragma once
#include <cstdint>
namespace fw::gw::guild {
// Midnight UTC boundaries, independent of local timezone and server uptime.
inline constexpr std::int64_t faction_day(std::int64_t seconds) { return seconds<0?0:seconds/86400; }
struct FactionDecay { std::uint32_t balance; std::int64_t day; };
inline constexpr FactionDecay decay_faction(std::uint32_t balance,std::int64_t saved_day,std::int64_t today) {
    if(today<=saved_day)return {balance,saved_day};
    auto days=today-saved_day;
    // Integer remaining balance rounds down, preserving a positive minimum1.
    // Timing/rate are documented; retail fractional rounding remains unverified.
    while(days-- && balance>1) { balance=static_cast<std::uint32_t>(std::uint64_t(balance)*9/10);if(!balance)balance=1; }
    return {balance,today};
}
}
