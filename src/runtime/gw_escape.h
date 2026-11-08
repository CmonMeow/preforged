#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <span>
namespace fw::gw::skills::effects {
inline std::optional<Time> escape(unsigned skill,unsigned rank){
 const auto* r=find_skill(skill);
 if(rank>20||(skill!=448&&skill!=3060)||!r||r->type!=3||r->attribute!=23||r->energy_cost()!=5||r->recharge!=12||r->duration0!=1||r->duration15!=8||r->scale0!=33||r->scale15!=33||r->bonus0!=75||r->bonus15!=75)return {};
 return 1000ull*interpolate(1,8,rank);
}
inline bool escape_active(std::span<const Entry> entries,Time now){
 for(const auto& e:entries)if((e.skill==448||e.skill==3060)&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires&&escape(e.skill,e.rank))return true;
 return false;
}
}
