#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"

namespace fw::gw::skills::effects {
struct WhirlingDefensePlan {
 unsigned rank=0,retaliation=5;Time duration=8000;double block=.75;float radius=166.f;
 bool operator==(const WhirlingDefensePlan&)const=default;
};
inline std::optional<WhirlingDefensePlan> whirling_defense(unsigned rank){
 const auto* r=find_skill(450);
 if(rank>20||!r||r->type!=3||r->attribute!=23||r->energy_cost()!=10||r->adrenaline||
    r->activation!=0||r->aftercast!=0||r->recharge!=60||r->duration0!=8||r->duration15!=20||
    r->scale0!=75||r->scale15!=75||r->bonus0!=5||r->bonus15!=11||
    r->caster_body_resource!=762||r->target_body_resource!=2077)return {};
 return WhirlingDefensePlan{rank,interpolate(5,11,rank),1000ull*interpolate(8,20,rank)};
}
}
