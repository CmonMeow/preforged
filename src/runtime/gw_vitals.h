#pragma once
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace fw::gw::vitals {
// Pinned37130 descriptor60bdd8 ->874b20 ->794750: a separate Energy
// ceiling/recovery curve, not current Energy or an integer capacity update.
// See overcast_curve_2022.py for original consumer and signed-curve evidence.
inline std::vector<std::uint8_t> energy_ceiling_curve_2022(std::uint32_t agent,
        unsigned nominal,double ceiling,double recovery_per_second){
    if(!agent||agent>65535||!nominal||nominal>1000000||!std::isfinite(ceiling)||
       ceiling< -1000000||ceiling>nominal||!std::isfinite(recovery_per_second)||
       recovery_per_second<0||recovery_per_second>nominal)
        throw std::invalid_argument("invalid Energy ceiling curve");
    const float fraction=static_cast<float>(ceiling)/static_cast<float>(nominal);
    const float rate=static_cast<float>(recovery_per_second)/static_cast<float>(nominal);
    std::vector<std::uint8_t> packet{156,0};
    for(auto value:{agent,std::bit_cast<std::uint32_t>(fraction),std::bit_cast<std::uint32_t>(rate)})
        for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    return packet;
}
inline std::vector<std::uint8_t> energy_regeneration_2022(std::uint32_t agent,int pips,std::uint32_t max_energy){
    if(!agent||!max_energy||max_energy>1000000||pips < -10||pips > 10)
        throw std::invalid_argument("invalid energy regeneration");
    // Build37130 descriptor60be14, callback874c20 ->794f60:
    // property43 selects energy(stat0),44 selects health(stat1).
    // 79a200 multiplies this fraction/second by energy capacity.
    // One energy pip is1/3 energy/second. Send after capacity updates.
    const float fraction=static_cast<float>(pips)/(3.0f*max_energy);
    std::vector<std::uint8_t> packet{161,0};
    for(auto value:{43u,agent,std::bit_cast<std::uint32_t>(fraction)})
        for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    return packet;
}
struct Resources {
    std::uint32_t health, max_health, energy, max_energy;
};
inline std::vector<std::uint8_t> level_2022(std::uint32_t agent,unsigned level){
    if(!agent||level<1||level>20)throw std::invalid_argument("invalid player level");
    // 37130 generic integer agent property: opcode158, property36 (level).
    std::vector<std::uint8_t> packet{158,0};
    for(auto value:{36u,agent,level})for(unsigned i=0;i<4;++i)
        packet.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    return packet;
}
inline std::vector<std::uint8_t> morale_2022(std::uint32_t agent,unsigned percent){
    if(!agent||percent<40||percent>110)throw std::invalid_argument("invalid player morale");
    // Descriptor 60bdcc -> 874b00 -> 794730 -> 7a0180. This updates
    // the per-agent morale table and emits UI event 10000047.
    std::vector<std::uint8_t> packet{155,0};
    for(auto value:{agent,percent})for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    return packet;
}
// Innate values only. Equipment, attributes and effects must modify these
// explicitly when those systems are implemented; profession alone adds none.
inline Resources base(unsigned level){
    if(level<1||level>20)throw std::invalid_argument("invalid player level for base resources");
    const auto hp=80+20*level;
    return {hp,hp,20,20};
}
inline std::vector<std::vector<std::uint8_t>> packets_2022(std::uint32_t agent,const Resources& r){
    if(!agent||!r.max_health||!r.max_energy||r.health>r.max_health||r.energy>r.max_energy)
        throw std::invalid_argument("invalid agent resources");
    std::vector<std::vector<std::uint8_t>> result;
    auto packet=[&](unsigned opcode,unsigned kind,std::uint32_t value){
        std::vector<std::uint8_t> p{static_cast<std::uint8_t>(opcode),static_cast<std::uint8_t>(opcode>>8)};
        for(auto field:{kind,agent,value})for(unsigned i=0;i<4;++i)p.push_back(static_cast<std::uint8_t>(field>>(8*i)));
        result.push_back(std::move(p));
    };
    // 37130 descriptors 60bdf0 / 60be14, callbacks 874b80 / 874c20.
    // Capacity precedes fraction: maxima alter the client's resource model.
    packet(158,41,r.max_energy);packet(158,42,r.max_health);
    packet(161,33,std::bit_cast<std::uint32_t>(float(r.energy)/r.max_energy));
    packet(161,34,std::bit_cast<std::uint32_t>(float(r.health)/r.max_health));
    return result;
}
inline std::vector<std::vector<std::uint8_t>> public_health_2022(std::uint32_t agent,const Resources& r){
    auto packets=packets_2022(agent,r);
    // Target bars need health, not the owner's energy or account progression.
    return {std::move(packets[1]),std::move(packets[3])};
}
}
