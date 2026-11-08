#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>
namespace fw::gw {
// Build37130 descriptor60bd6c ->874990 ->794230. Five counted DWORD
// bitfields: four mission completion/bonus arrays, then unlocked maps.
inline std::vector<std::uint8_t> map_unlocks_2022(std::span<const std::uint32_t> maps){
    std::array<std::uint32_t,32> bits{};
    for(auto map:maps){if(map>=1024)throw std::invalid_argument("map unlock exceeds retail bitfield");bits[map/32]|=1u<<(map%32);}
    std::vector<std::uint8_t> packet{147,0};
    for(unsigned i=0;i<4;++i){packet.push_back(0);packet.push_back(0);}
    packet.push_back(32);packet.push_back(0);
    for(auto word:bits)for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(word>>(i*8)));
    return packet;
}
}
