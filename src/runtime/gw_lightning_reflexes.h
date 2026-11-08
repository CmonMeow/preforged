#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <span>
namespace fw::gw::skills::effects {
inline std::optional<Time> lightning_reflexes(unsigned skill,unsigned rank){
 const auto* r=find_skill(skill);
 if(rank>20||(skill!=453&&skill!=3141)||!r||r->type!=3||r->attribute!=23||r->energy_cost()!=10||r->scale0!=75||r->scale15!=75||r->bonus0!=33||r->bonus15!=33||r->duration0!=5||r->duration15!=11||r->recharge!=(skill==453?30:45))return {};
 return 1000ull*interpolate(5,11,rank);
}
inline bool lightning_reflexes_active(std::span<const Entry> entries,Time now){
 for(const auto& e:entries)if((e.skill==453||e.skill==3141)&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires&&lightning_reflexes(e.skill,e.rank))return true;
 return false;
}
}
