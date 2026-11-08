#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>
#include <vector>
namespace fw::gw::skills::effects {
struct MindShockPlan {
 unsigned skill=226,rank=0;double damage=10;unsigned penetration=25;
 std::uint64_t knockdown_ms=2000;float range=1248;
 auto operator<=>(const MindShockPlan&)const=default;
};
inline std::optional<MindShockPlan> mind_shock(unsigned skill,unsigned rank=0){
 if((skill!=226&&skill!=2804)||rank>20)return {};
 const auto* row=find_skill(skill);
 if(!row||row->type!=5||row->attribute!=8||row->scale0!=10||row->scale15!=50||row->encoded_energy!=5||
    row->overcast!=(skill==226?5u:10u)||row->pvp_skill!=(skill==226?2804u:226u))return {};
 return MindShockPlan{skill,rank,damage_scale(10,50,rank)};
}
struct MindShockResult {
 bool accepted=false,energy_advantage=false,knockdown=false;
 combat::Actor actor;std::vector<combat::Hit> hits;
};
// Inputs use the existing authoritative signed300000-units-per-Energy model.
// Unknown Energy must not silently turn a conditional spell into a base hit.
// World must settle payment/regeneration/drains and acquire both lifetimes
// before calling this resolver; it owns immunity,control and publication.
inline MindShockResult mind_shock_target(const MindShockPlan& plan,combat::Actor actor,
 const combat::conditions::State& conditions,unsigned level,std::optional<std::int64_t> caster_energy,
 std::optional<std::int64_t> target_energy,std::uint64_t now,const combat::ArmorSequence& armor_at_hit={},const combat::DamageProjection& projection={}){
 const auto canonical=mind_shock(plan.skill,plan.rank);
 constexpr std::int64_t limit=1000000ll*300000;
 if(!canonical||*canonical!=plan||!caster_energy||!target_energy||*caster_energy< -limit||*caster_energy>limit||
    *target_energy< -limit||*target_energy>limit||!actor.lightning_armor)return {};
 actor.lightning_armor=conditions.armor(*actor.lightning_armor,now);
 MindShockResult result;result.actor=actor;result.energy_advantage=*caster_energy>*target_energy;
 const combat::Damage damage{combat::Kind::lightning,plan.damage,level,plan.skill,plan.penetration};
 std::optional<combat::Hit> previous_damage;
 for(unsigned n=0;n<(result.energy_advantage?2u:1u);++n){
  if(armor_at_hit){const auto rating=armor_at_hit(previous_damage?&*previous_damage:nullptr);if(!rating)return {};
   result.actor.lightning_armor=conditions.armor(*rating,now);}
  const auto step=combat::project_damage(result.actor,damage,projection);if(!step)return {};
  result.hits.push_back(step->incoming);previous_damage=step->projected;
  if(result.actor.health->current<=0)break;
 }
 result.accepted=true;result.knockdown=result.energy_advantage&&result.actor.health->current>0;
 return result;
}
}
