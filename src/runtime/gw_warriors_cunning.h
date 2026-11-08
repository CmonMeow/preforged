#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <span>
namespace fw::gw::skills::effects {
inline std::optional<Time> warriors_cunning(unsigned rank){
 const auto* r=find_skill(362);
 if(rank>20||!r||r->type!=16||r->attribute!=17||r->energy_cost()!=10||r->recharge!=60||r->activation!=0||r->aftercast!=0||r->duration0!=5||r->duration15!=11)return {};
 return 1000ull*interpolate(5,11,rank);
}
inline bool warriors_cunning_active(std::span<const Entry> entries,unsigned weapon_type,Time now){
 // GWCA ItemType: axe2,hammer15,sword27,daggers32,scythe35.
 if(weapon_type!=2&&weapon_type!=15&&weapon_type!=27&&weapon_type!=32&&weapon_type!=35)return false;
 for(const auto& e:entries)if(e.skill==362&&e.kind==Category::other&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires&&warriors_cunning(e.rank))return true;
 return false;
}
}
