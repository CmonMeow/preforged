#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace capture {
// Exact pinned modern client 54db70: ordinary travel uses 186; 183 takes a
// guild-hall identifier. GWCA ef3d3670 still sends this structure to 183.
inline constexpr std::uint32_t ordinary_travel_message=0x10000186;
struct TravelRequest {std::uint32_t map;std::int32_t region;std::uint32_t language;std::uint32_t district;};
static_assert(sizeof(TravelRequest)==16);
inline bool travel_layout_matches(std::span<const std::uint8_t> code){
    constexpr std::array<std::uint8_t,23> expected={
        0x8b,0x45,0x0c,0x89,0x45,0xf0,0x8b,0x45,0x10,0x89,0x45,0xf4,
        0x8b,0x45,0x14,0x89,0x45,0xf8,0x8d,0x45,0xec,0x6a,0x00};
    if(code.size()<32)return false;
    for(unsigned i=0;i<expected.size();++i)if(code[i]!=expected[i])return false;
    return code[23]==0x50&&code[24]==0x89&&code[25]==0x75&&code[26]==0xec
        &&code[27]==0x68&&code[28]==0x86&&code[29]==0x01&&code[30]==0x00&&code[31]==0x10;
}
}
