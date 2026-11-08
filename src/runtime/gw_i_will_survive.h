#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
struct SurvivalPlan {unsigned rank=0;Time duration=5000;int per_condition=3;bool operator==(const SurvivalPlan&)const=default;};
inline std::optional<SurvivalPlan> i_will_survive(unsigned rank){
 const auto* r=find_skill(368);
 if(rank>20||!r||r->type!=15||r->attribute!=17||r->energy_cost()!=5||r->activation!=0||r->aftercast!=0||
    r->recharge!=30||r->scale0!=3||r->scale15!=3||r->duration0!=5||r->duration15!=11||
    r->caster_body_resource!=627||r->target_body_resource!=2077)return {};
 return SurvivalPlan{rank,1000ull*interpolate(5,11,rank),3};
}
// Count current conditions, not an activation-time snapshot; the World supplies
// each settled segment's count and sums all regeneration before the shared cap.
inline int survival_pips(const Registry& registry,Scope owner,unsigned conditions,Time now){
 if(conditions>10)return 0;
 const auto entry=registry.newest_skill(owner,368,now);
 return entry&&entry->cause==owner.actor&&entry->kind==Category::other?3*static_cast<int>(conditions):0;
}
}
