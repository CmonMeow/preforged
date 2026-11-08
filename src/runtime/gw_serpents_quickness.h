#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
struct SerpentsQuicknessPlan {
 unsigned rank=0;Time duration=15000;unsigned reduction=33;
 auto operator<=>(const SerpentsQuicknessPlan&)const=default;
};
inline std::optional<SerpentsQuicknessPlan> serpents_quickness(unsigned rank){
 const auto* row=find_skill(456);
 if(rank>20||!row||row->type!=3||row->profession!=2||row->attribute!=24||
    row->energy_cost()!=5||row->activation||row->aftercast||row->recharge!=45||
    row->scale0!=33||row->scale15!=33||row->duration0!=15||row->duration15!=30||
    row->caster_body_resource!=768)return {};
 return SerpentsQuicknessPlan{rank,1000ull*interpolate(15,30,rank),33};
}
// Completion-time reduction: it does not change an already running timer,
// its own recharge, or a replacing stance. Interrupted uses do not complete.
inline bool serpents_quickness_reduces(const SkillRecord& skill,const Entry& source,Time now){
 return source.skill==456&&source.kind==Category::stance&&source.instance&&source.cause&&
   source.applied<=now&&source.expires&&now<*source.expires&&serpents_quickness(source.rank)&&
   skill.id!=456&&skill.type!=3;
}
inline bool serpents_quickness_ends(double health,unsigned maximum){
 return maximum&&std::isfinite(health)&&health<double(maximum)/2.;
}
}
