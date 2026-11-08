#pragma once
#include <cstdint>
#include <optional>
#include <span>

namespace fw::gw::skills {
struct UseRequest {
    std::uint16_t opcode=0;
    std::uint32_t skill=0,copy=0,target=0;
    std::uint8_t call=0;
    friend bool operator==(const UseRequest&,const UseRequest&)=default;
};
// Build37130 original senders875fd0 (attack37) and876730 (ordinary68)
// serialize identical ordered fields: WORD opcode, DWORD skill/copy/target,
// BYTE call. Descriptor tags differ but do not change those field positions.
// Parse one already-framed packet; reject trailing data so coalesced requests
// cannot silently become one request. Catalog/ownership policy belongs to the
// authenticated caller, not this wire parser. Preserve the full native call byte.
inline std::optional<UseRequest> parse_use_request(std::span<const std::uint8_t> bytes){
    if(bytes.size()!=15)return std::nullopt;
    const auto opcode=std::uint16_t((bytes[0]|(unsigned(bytes[1])<<8))&0x7fff);
    if(opcode!=37&&opcode!=68)return std::nullopt;
    const auto read32=[&](std::size_t at){
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[at+i])<<(8*i);
        return value;
    };
    return UseRequest{opcode,read32(2),read32(6),read32(10),bytes[14]};
}
}
