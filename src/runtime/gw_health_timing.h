#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
namespace fw::gw::combat::health_timing {
// Millisecond clock normalization. Repeated fractional regeneration must not
// leave an arithmetic residue alive for an extra millisecond.
inline std::optional<std::uint64_t> death_delay(double health,int pips){
    if(!std::isfinite(health)||health<=0||pips>=0||pips< -10)return {};
    const auto delay=std::ceil(health*1000./(-2.*pips)-1e-7);
    if(!std::isfinite(delay)||delay>=double(UINT64_MAX))return {};
    return std::max<std::uint64_t>(1,static_cast<std::uint64_t>(std::max(0.,delay)));
}
inline bool zero_residue(double health,int pips){return pips<0&&health>0&&health<1e-7;}
}
