#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"

namespace fw::gw::skills::effects {
struct GladiatorDefensePlan {
 unsigned rank=0,retaliation=5;Time duration=5000;double block=.75;
 bool operator==(const GladiatorDefensePlan&)const=default;
};
inline std::optional<GladiatorDefensePlan> gladiator_defense(unsigned rank){
 const auto* r=find_skill(372);
 if(rank>20||!r||r->type!=3||r->attribute!=21||r->energy_cost()!=5||r->adrenaline||
    r->activation!=0||r->aftercast!=0||r->recharge!=30||r->duration0!=5||r->duration15!=11||
    r->scale0!=75||r->scale15!=75||r->bonus0!=5||r->bonus15!=35||
    r->caster_body_resource!=631||r->target_body_resource!=2077)return {};
 return GladiatorDefensePlan{rank,interpolate(5,35,rank),1000ull*interpolate(5,11,rank)};
}
}
