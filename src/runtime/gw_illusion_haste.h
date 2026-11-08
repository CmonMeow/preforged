#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct IllusionHastePlan {
    unsigned skill=37,rank=0;std::uint64_t duration=0,end_cripple=3000;float multiplier=1.33f;bool cleanse=true;
    auto operator<=>(const IllusionHastePlan&)const=default;
};
inline std::optional<IllusionHastePlan> illusion_haste(unsigned skill,unsigned rank=0,bool pvp=false){
    if(skill==37&&pvp)skill=3373;
    if(rank>20||(skill!=37&&skill!=3373))return {};
    const auto* row=find_skill(skill);if(!row||row->type!=6||row->attribute!=1||row->scale0!=33||row->scale15!=33)return {};
    return IllusionHastePlan{skill,rank,1000ull*interpolate(row->duration0,row->duration15,rank),3000,1.33f,skill==37};
}
}
