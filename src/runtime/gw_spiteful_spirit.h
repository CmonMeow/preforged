#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct SpitefulSpiritPlan {
 unsigned rank=0,damage=0;std::uint64_t duration=0;float range=1248,radius=166;
 auto operator<=>(const SpitefulSpiritPlan&)const=default;
};
inline std::optional<SpitefulSpiritPlan> spiteful_spirit(unsigned rank){
 const auto* row=find_skill(121);
 if(rank>20||!row||row->type!=4||row->profession!=4||row->attribute!=7||row->special!=4||
    row->energy_cost()!=15||row->health_sacrifice||row->activation!=2.f||row->aftercast!=.75f||row->recharge!=10||
    row->duration0!=8||row->duration15!=20||row->scale0!=5||row->scale15!=35||
    row->caster_body_resource!=2077||row->target_body_resource!=227)return {};
 return SpitefulSpiritPlan{rank,interpolate(5,35,rank),std::uint64_t(interpolate(8,20,rank))*1000};
}
}
