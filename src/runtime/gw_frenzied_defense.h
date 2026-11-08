#pragma once
#include "gw_effect_registry.h"
#include <span>
namespace fw::gw::skills::effects {
struct FrenziedDefensePlan {
 unsigned rank=0,energy=5;Time duration=8000,recharge=10000;
 double block=.75,strike_multiplier=2;
 bool operator==(const FrenziedDefensePlan&)const=default;
};
inline std::optional<FrenziedDefensePlan> frenzied_defense(unsigned rank){
 const auto* r=find_skill(1700);
 if(rank>20||!r||r->type!=3||r->profession!=1||r->attribute!=51||r->energy_cost()!=5||
    r->adrenaline||r->activation||r->aftercast||r->recharge!=10||r->duration0!=8||r->duration15!=8||
    r->scale0!=75||r->scale15!=75||r->bonus0||r->bonus15||
    r->caster_body_resource!=1666||r->target_body_resource!=2077)return {};
 return FrenziedDefensePlan{rank};
}
// Supplied entries belong to the recipient's current actor lifetime. Strike
// damage doubles before caps/reductions; degeneration,sacrifice and life steal
// do not pass through this modifier. This stance grants no attack-speed bonus.
inline std::optional<bool> frenzied_defense_active(std::span<const Entry> entries,Time now){
 bool active=false;
 for(const auto& e:entries){
  if(e.skill!=1700||e.applied>now||(e.expires&&now>=*e.expires))continue;
  if(e.kind!=Category::stance||!e.instance||!e.cause||!e.expires||!frenzied_defense(e.rank))return {};
  active=true;
 }
 return active;
}
}
