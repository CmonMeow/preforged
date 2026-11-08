#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct BlindnessPlan {unsigned skill=0,rank=0;float range=1248,radius=0;std::uint64_t duration=0;bool blind_caster=false;auto operator<=>(const BlindnessPlan&)const=default;};
inline std::optional<BlindnessPlan> blindness(unsigned skill,unsigned rank=0){
    if(rank>20)return {};
    if(skill==58)return BlindnessPlan{58,rank,144,0,15000,true}; // Signet of Midnight: unlinked touch signet.
    if(skill==220)return BlindnessPlan{220,rank,1248,0,std::uint64_t(interpolate(3,8,rank))*1000};
    if(skill==424)return BlindnessPlan{424,rank,144,166,std::uint64_t(interpolate(3,15,rank))*1000};
    return {};
}
inline bool valid_blindness(const BlindnessPlan& p){const auto expected=blindness(p.skill,p.rank);return expected&&*expected==p;}
}
