#pragma once
#include "gw_player_movement.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace fw::gw::agent {
using Packet=std::vector<std::uint8_t>;
// Pinned37130:241->8756d0->796ca0 sets the local comparison token;
// 47 updates an agent token. Distinct nonreserved tokens are foes.
inline Packet team_token_2022(std::uint32_t token){
    Packet out{241,0};for(unsigned i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(token>>(8*i)));return out;
}
inline Packet allegiance_token_2022(std::uint32_t id,std::uint32_t token){
    if(!id)throw std::invalid_argument("invalid team agent");
    Packet out{47,0};for(auto value:{id,token})for(unsigned i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));return out;
}
// Pinned5bbf10 ->5c33b0: absolute millisecond epoch for both agent timers.
// Reset before creating actors; actor/history timestamps are not rebased.
inline Packet instance_timer_2022(std::uint32_t milliseconds){Packet out{31,0};for(unsigned i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(milliseconds>>(8*i)));return out;}
// 5bbe70 -> 5c2e90 advances authority by elapsed milliseconds.
inline Packet movement_tick_2022(std::uint32_t elapsed_ms){
    if(elapsed_ms>0x7fffffffu)throw std::invalid_argument("invalid movement tick");
    Packet out{30,0};
    for(unsigned i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(elapsed_ms>>(8*i)));
    return out;
}
// 37130 descriptor RVA 59b11c -> handler 5bc8a0 -> 5c17d0.
// Destination (41) does not initialize the separate movement multiplier.
// 43 final BYTE is gait:5c17d0 stores agent+c4;5c14a0 direction1 copies
// normalized route displacement into body-facing bc/c0, direction0 preserves it.
// Descriptor59b0ec [39,DWORD,float],5bc5d0->5c1750 accepts nonnegative
// base speed. Boost changes this value;43 remains normalized0.01..1.
inline Packet movement_base_speed_2022(std::uint32_t id,float speed){
    if(!id||!std::isfinite(speed)||speed<0.f)throw std::invalid_argument("invalid base movement speed");
    Packet out{39,0};for(auto value:{id,std::bit_cast<std::uint32_t>(speed)})for(unsigned i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));return out;
}
inline Packet movement_speed_2022(std::uint32_t id,float multiplier=1.f,std::uint8_t direction=0){
    if(!id||!std::isfinite(multiplier)||multiplier<.01f||multiplier>1.f||direction>8)
        throw std::invalid_argument("invalid movement speed");
    Packet out{43,0};
    for(auto value:{id,std::bit_cast<std::uint32_t>(multiplier)})
        for(unsigned i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    out.push_back(direction);return out;
}
// Pinned37130 descriptor59b140: [46,DWORD actor,DWORD angle bits,DWORD speed bits].
// Handler5bcb30 passes FLOAT+8/+c to5c1b10: angle atagent+cc,
// angular speed at+c8; turn duration=abs(angle-current)*1000/speed.
inline Packet facing_2022(std::uint32_t id,float angle,float radians_per_second=2.094395f){
 if(!id||!std::isfinite(angle)||!std::isfinite(radians_per_second)||radians_per_second<=0)
  throw std::invalid_argument("invalid agent facing");
 Packet out{46,0};for(auto value:{id,std::bit_cast<std::uint32_t>(angle),std::bit_cast<std::uint32_t>(radians_per_second)})
  for(unsigned i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));return out;
}
// Minimum stationary living-agent record. Model references a previously
// populated NPC table (0x20000000) or player table (0x30000000).
inline Packet spawn_2022(std::uint32_t id,std::uint32_t model,float x,float y,
        std::uint16_t plane=0,float direction_x=1,float direction_y=0,std::uint32_t team_token=0){
    const auto kind=model&0xffff0000u;
    if(!id||(kind!=0x20000000u&&kind!=0x30000000u)||
       !std::isfinite(x)||!std::isfinite(y)||!std::isfinite(direction_x)||!std::isfinite(direction_y)||
       std::abs(direction_x*direction_x+direction_y*direction_y-1.f)>0.001f)
        throw std::invalid_argument("invalid living agent spawn");
    Packet out;
    auto put=[&](std::uint32_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));};
    auto real=[&](float value){put(std::bit_cast<std::uint32_t>(value),4);};
    put(32,2);put(id,4);put(model,4);put(1,1);put(0,1);
    real(x);real(y);put(plane,2);real(direction_x);real(direction_y);put(0,1);
    real(kind==0x30000000u?movement::player_run_speed:220.f);real(1); // Player capture baseline; retain separate NPC fallback.
    put(0,4);put(team_token,4);for(unsigned i=2;i<7;++i)put(0,4);
    for(unsigned i=0;i<4;++i)real(0);
    put(0,2);put(0,4);real(0);real(0);put(0,2);
    return out;
}
}


