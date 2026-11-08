#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
struct ToLimitPlan {
    unsigned rank=0,health_bonus=10,maximum_foes=1,energy=5;
    Time duration=10000,recharge=15000;
    float radius=1012;
    auto operator<=>(const ToLimitPlan&)const=default;
};
inline std::optional<ToLimitPlan> to_limit(unsigned rank){
    const auto* row=find_skill(316);
    if(rank>20||!row||row->profession!=1||row->attribute!=21||row->type!=15||row->special||row->energy_cost()!=5||row->activation!=0||row->aftercast!=0||row->recharge!=15||
       row->scale0!=10||row->scale15!=60||row->bonus0!=1||row->bonus15!=6||row->duration0!=10||row->duration15!=20||row->caster_body_resource!=562||row->target_body_resource!=2077)return {};
    return ToLimitPlan{rank,interpolate(10,60,rank),interpolate(1,6,rank),5,Time(interpolate(10,20,rank))*1000};
}
struct ToLimitResult {unsigned health_bonus,adrenaline_units;Time duration;};
// Both components are mandatory. This proposal does not mutate a Health base,
// heal,grant party Health,or imply that the reviewed foe count came from UI.
inline std::optional<ToLimitResult> to_limit_result(const ToLimitPlan& plan,unsigned reviewed_foes){
    const auto canonical=to_limit(plan.rank);if(!canonical||*canonical!=plan)return {};
    return ToLimitResult{plan.health_bonus,25*std::min(reviewed_foes,plan.maximum_foes),plan.duration};
}
}
