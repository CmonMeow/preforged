#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <span>
namespace fw::gw::skills::effects {
inline std::optional<Time> flail(unsigned rank){
 const auto* r=find_skill(1404);
 if(rank>20||!r||r->type!=3||r->attribute!=17||r->adrenaline!=100||r->energy_cost()!=0||r->duration0!=1||r->duration15!=15||r->scale0!=33||r->scale15!=33||r->bonus0!=33||r->bonus15!=33)return {};
 return 1000ull*interpolate(1,15,rank);
}
inline bool flail_active(std::span<const Entry> entries,Time now){
 for(const auto& e:entries)if(e.skill==1404&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires&&flail(e.rank))return true;
 return false;
}
}
