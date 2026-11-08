#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
struct WatchfulHealingPlan {
    unsigned rank=0,pips=1,gain=30,energy=5;
    Time duration=10000,activation=1000,aftercast=750,recharge=10000;
    auto operator<=>(const WatchfulHealingPlan&)const=default;
};
inline std::optional<WatchfulHealingPlan> watchful_healing(unsigned rank){
    const auto* row=find_skill(1392);
    if(rank>20||!row||row->type!=6||row->profession!=3||row->attribute!=16||row->special||
       row->energy_cost()!=5||row->health_sacrifice||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=10||
       row->scale0!=1||row->scale15!=4||row->bonus0!=30||row->bonus15!=120||row->duration0!=10||row->duration15!=10||
       row->caster_body_resource!=2077||row->target_body_resource!=1647)return {};
    return WatchfulHealingPlan{rank,interpolate(1,4,rank),interpolate(30,120,rank)};
}
// Early removal grants Health rather than healing. Natural expiry/death cannot
// grant it. Renewal uses the shared replacement path and never calls this end.
inline std::optional<unsigned> watchful_early_gain(const Entry& entry,Time now,bool alive){
    const auto plan=watchful_healing(entry.rank);
    if(!plan||entry.skill!=1392||entry.kind!=Category::enchantment||!entry.instance||!entry.cause||
       !entry.expires||*entry.expires<=entry.applied||now<entry.applied)return {};
    return alive&&now<*entry.expires?plan->gain:0;
}
}
