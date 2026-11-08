#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
struct UnnaturalPlan {unsigned rank=0,primary=15,adjacent=5;float range=1248,radius=166;unsigned skill=934;bool pvp=false;bool operator==(const UnnaturalPlan&)const=default;};
inline std::optional<UnnaturalPlan> unnatural_signet(unsigned rank,bool pvp=false){
 const auto* r=find_skill(pvp?3188u:934u);
 if(rank>20||!r||r->type!=7||r->attribute!=2||r->energy_cost()!=0||r->activation!=1.f||r->aftercast!=.75f||
    r->recharge!=(pvp?20u:10u)||r->scale0!=(pvp?10u:15u)||r->scale15!=(pvp?60u:75u)||r->bonus0!=(pvp?10u:5u)||r->bonus15!=(pvp?60u:50u)||
    r->caster_body_resource!=2077||r->target_body_resource!=982)return {};
 return UnnaturalPlan{rank,interpolate(pvp?10u:15u,pvp?60u:75u,rank),interpolate(pvp?10u:5u,pvp?60u:50u,rank),1248,166,pvp?3188u:934u,pvp};
}
}
