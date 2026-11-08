#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct RottingFleshPlan {
 unsigned rank=0;std::uint64_t duration=0;float range=1248;
 auto operator<=>(const RottingFleshPlan&)const=default;
};
inline std::optional<RottingFleshPlan> rotting_flesh(unsigned rank){
 const auto* row=find_skill(106);
 if(rank>20||!row||row->type!=5||row->profession!=4||row->attribute!=5||row->special||
    row->energy_cost()!=15||row->health_sacrifice||row->activation!=3.f||row->aftercast!=.75f||row->recharge!=3||
    row->bonus0!=10||row->bonus15!=25||row->caster_body_resource!=2077||row->target_body_resource!=205)return {};
 return RottingFleshPlan{rank,std::uint64_t(interpolate(10,25,rank))*1000};
}
}
