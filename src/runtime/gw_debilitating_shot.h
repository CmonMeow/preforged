#pragma once
#include "gw_skill_effect_plan.h"
namespace fw::gw::combat::weapon {
inline std::optional<unsigned> debilitating_shot(unsigned rank){
 const auto* row=skills::find_skill(406);
 if(rank>20||!row||row->type!=14||row->attribute!=25||row->energy_cost()!=10||row->activation!=0.f||row->aftercast!=0.f||row->recharge!=10||row->scale0!=1||row->scale15!=10)return {};
 return skills::effects::interpolate(1,10,rank);
}
}
