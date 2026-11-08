#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <stdexcept>
namespace fw::gw::callout {
// Pinned 37130 sender 875e90: BYTE kind, DWORD argument. Argument semantics
// vary: gmlevelprogress sends local agent, gmskbar sends skill, target uses agent.
// Private 37130 UI captures: Ctrl-health sends kind5, Ctrl-energy sends kind1,
// both with the local agent ID. Morale sender: 50f900 -> kind7.
enum class Kind:std::uint8_t { unknown0=0,energy=1,experience=2,following=3,
    effect4=4,health=5,effect6=6,morale=7,effect8=8,skill=9,target=10 };
struct Request { Kind kind;std::uint32_t argument; };
// SMSG 52 descriptor RVA 626024, callback 8736c0 -> 7d7fe0.
// Requires caller membership in the receiving client's local party.
inline std::vector<std::uint8_t> target_response(std::uint16_t caller,std::uint32_t target){
    if(!caller)throw std::invalid_argument("target call requires player identity");
    std::vector<std::uint8_t> packet{52,0,static_cast<std::uint8_t>(caller),static_cast<std::uint8_t>(caller>>8)};
    for(unsigned shift=0;shift<32;shift+=8)packet.push_back(static_cast<std::uint8_t>(target>>shift));
    return packet;
}
inline std::optional<Request> parse(std::span<const std::uint8_t> packet){
    if(packet.size()!=7||packet[0]!=33||(packet[1]&0x7f)!=0||packet[2]>10)return {};
    return Request{static_cast<Kind>(packet[2]),std::uint32_t(packet[3])|
        (std::uint32_t(packet[4])<<8)|(std::uint32_t(packet[5])<<16)|(std::uint32_t(packet[6])<<24)};
}
inline const char* argument_kind(Kind kind){
    switch(kind){case Kind::skill:return "skill";
    case Kind::energy:case Kind::health:case Kind::experience:case Kind::morale:case Kind::target:case Kind::following:return "agent";
    default:return "unverified";}
}
}
