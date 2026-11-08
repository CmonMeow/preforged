#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct DrainDelusionsPlan {
    unsigned rank=0,loss=1,multiplier=4;float range=1248;
    auto operator<=>(const DrainDelusionsPlan&)const=default;
};
inline std::optional<DrainDelusionsPlan> drain_delusions(unsigned rank){
    const auto* row=find_skill(1337);
    if(rank>20||!row||row->type!=5||row->profession!=5||row->attribute!=3||row->energy_cost()!=5||
       row->activation!=.25f||row->aftercast!=.75f||row->recharge!=12||row->scale0!=1||row->scale15!=5||
       row->bonus0!=4||row->bonus15!=4||row->caster_body_resource!=2077||row->target_body_resource!=49)return {};
    return DrainDelusionsPlan{rank,interpolate(1,5,rank),4};
}
}
