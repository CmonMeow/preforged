#pragma once
#include "gw_effect_registry.h"
#include <span>
namespace fw::gw::skills::effects {
// Local Flurry revision2659834: only weapon base damage loses25percent;
// attack-skill bonus and separate skill damage are unchanged.
inline bool flurry_plan(){const auto* r=find_skill(344);return r&&r->type==3&&r->energy_cost()==5&&r->recharge==5&&r->duration0==5&&r->duration15==5;}
inline bool flurry_active(std::span<const Entry> entries,Time now){
 if(!flurry_plan())return false;
 for(const auto& e:entries)if(e.skill==344&&e.kind==Category::stance&&e.instance&&e.cause&&e.rank<=20&&e.applied<=now&&e.expires&&now<*e.expires)return true;
 return false;
}
}
