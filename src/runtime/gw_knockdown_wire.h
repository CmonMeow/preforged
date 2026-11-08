#pragma once
#include <bit>
#include <cmath>
#include <cstdint>
#include <vector>
#include <stdexcept>
namespace fw::gw::combat {
// Pinned161 descriptor60be14: kindDWORD, actorDWORD, durationFLOAT.
// Original874c20 ->794f60 kind63 ->764520 ->77a7d0 agent action.
inline std::vector<uint8_t> knockdown_2022(uint32_t actor,float seconds){
 if(!actor||!std::isfinite(seconds)||seconds<=0||seconds>4)throw std::invalid_argument("invalid reviewed knockdown duration");
 std::vector<uint8_t> packet;const auto append=[&](uint32_t n,unsigned size){for(unsigned i=0;i<size;++i)packet.push_back(static_cast<uint8_t>(n>>(8*i)));};append(161,2);append(63,4);append(actor,4);append(std::bit_cast<uint32_t>(seconds),4);return packet;
}
}
