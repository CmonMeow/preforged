#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
namespace fw::gw::compass {
// 37130 CMSG41: DWORD stroke, WORD count, count packed short-x/short-y.
// SMSG144 adds the authenticated WORD player number before that payload.
inline std::optional<std::vector<std::uint8_t>> response_2022(
        std::uint32_t player,std::span<const std::uint8_t> request){
    if(!player||player>65535||request.size()<8)return std::nullopt;
    const auto opcode=std::uint16_t(request[0])|(std::uint16_t(request[1])<<8);
    const auto count=std::uint16_t(request[6])|(std::uint16_t(request[7])<<8);
    if((opcode&0x7fff)!=41||count<1||count>16||request.size()!=8+4*count)return std::nullopt;
    std::vector<std::uint8_t> packet{144,0,static_cast<std::uint8_t>(player),static_cast<std::uint8_t>(player>>8)};
    packet.insert(packet.end(),request.begin()+2,request.end());
    return packet;
}
}
