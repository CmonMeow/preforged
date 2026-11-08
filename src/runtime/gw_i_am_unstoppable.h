#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"

namespace fw::gw::skills::effects {
struct UnstoppablePlan {
 unsigned norn_rank=0,armor_after_cap=24;
 Time duration=16000,recharge=30000;
 bool operator==(const UnstoppablePlan&)const=default;
};
inline std::optional<UnstoppablePlan> i_am_unstoppable(unsigned rank){
 const auto* row=find_skill(2356);
 if(rank>10||!row||row->campaign!=4||row->profession!=0||row->attribute!=51||row->type!=15||
    row->energy_cost()!=5||row->adrenaline||row->health_sacrifice||row->activation!=0||row->aftercast!=0||
    row->recharge!=30||row->scale0!=24||row->scale15!=24||row->bonus0||row->bonus15||
    row->duration0!=16||row->duration15!=20||row->caster_body_resource!=1908||row->target_body_resource!=2077)return {};
 // The archived description indexes this PvE skill by the ten earned Norn
 // ranks. Keep the rank in the plan so the World can reject forged duration.
 const auto seconds=16u+static_cast<unsigned>(std::lround(4.0*rank/10.0));
 return UnstoppablePlan{rank,24,1000ull*seconds,30000};
}
inline bool unstoppable_active(const Registry& registry,Scope owner,Time now){
 const auto entry=registry.newest_skill(owner,2356,now);
 return entry&&entry->cause==owner.actor&&entry->kind==Category::other&&
    entry->expires&&*entry->expires>=entry->applied&&
    i_am_unstoppable(entry->rank)&&*entry->expires-entry->applied==i_am_unstoppable(entry->rank)->duration;
}
inline unsigned unstoppable_post_cap_armor(const Registry& registry,Scope owner,Time now){
 return unstoppable_active(registry,owner,now)?24u:0u;
}
}
