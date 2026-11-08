#pragma once
#include "gw_skill_effect_plan.h"
namespace fw::gw::combat::weapon {
struct PrecisionShotPlan {unsigned rank=0,bonus=3;std::uint64_t windup=1000;auto operator<=>(const PrecisionShotPlan&)const=default;};
inline std::optional<PrecisionShotPlan> precision_shot(unsigned rank){
 const auto* row=skills::find_skill(400);
 if(rank>20||!row||row->type!=14||row->attribute!=25||row->energy_cost()!=10||row->activation!=1.f||row->aftercast!=1.f||row->recharge!=6||row->scale0!=3||row->scale15!=10)return {};
 return PrecisionShotPlan{rank,skills::effects::interpolate(3,10,rank),1000};
}
}
