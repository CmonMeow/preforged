#pragma once
#include <cstdint>
#include <optional>
#include <span>
namespace fw::gw::combat {
struct AttackRequest {std::uint32_t target;std::uint8_t call;};
// Original875f80/descriptor60b6f0:36 DWORD target BYTE call. Unlike attack
// skills37, ordinary attacks contain no skill ID or copy index.
inline std::optional<AttackRequest> parse_attack(std::span<const std::uint8_t> bytes){
 if(bytes.size()!=7||((bytes[0]|unsigned(bytes[1])<<8)&0x7fff)!=36)return {};
 std::uint32_t target=0;for(unsigned i=0;i<4;++i)target|=std::uint32_t(bytes[2+i])<<(8*i);
 return AttackRequest{target,bytes[6]};
}
}
