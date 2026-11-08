#pragma once
#include <cmath>
#include <optional>
#include <span>
namespace fw::gw::combat::weapon {
// Independent chances combine by multiplying the chance to hit,not by
// adding miss percentages. The caller owns the server-side random draw.
inline std::optional<double> combined_miss_chance(std::span<const double> chances){
    double hit=1;
    for(double chance:chances){
        if(!std::isfinite(chance)||chance<0||chance>1)return {};
        hit*=1-chance;
    }
    return 1-hit;
}
inline std::optional<bool> attack_misses(double roll,bool blind,double additional=0){
    if(!std::isfinite(roll)||roll<0||roll>=1)return {};
    const double chances[]={blind?.9:0,additional};
    const auto chance=combined_miss_chance(chances);
    return chance?std::optional(roll<*chance):std::nullopt;
}
}
