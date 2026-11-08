#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include "gw_reversal_fortune.h"
#include <functional>
namespace fw::gw::skills::effects {
// A copied preflight provider may update armor after the preceding resolved
// hit. It must not mutate live World ownership while calculating a batch.
using EarthArmorSequence=combat::ArmorSequence;
using EarthDamageResolver=std::function<combat::Hit(combat::Actor&,const combat::Damage&)>;
struct ShockwaveTarget {combat::Actor actor;combat::conditions::State conditions;};
using EarthConditionReaction=std::function<bool(ShockwaveTarget&,combat::conditions::Kind)>;
struct ShockwaveResult {bool accepted=false;ShockwaveTarget after;std::vector<combat::Hit> hits;std::vector<combat::conditions::Kind> applied_conditions;std::vector<std::optional<ReversalDamageReceipt>> reversals;};
inline bool valid_shockwave_plan(const AoEPlan& plan){const auto reviewed=caster_earth_aoe(937,plan.rank);if(!reviewed||plan.skill!=937||plan.count!=3)return false;for(unsigned n=0;n<3;++n){const auto& a=plan.strikes[n];const auto& b=reviewed->strikes[n];if(a.radius!=b.radius||a.earth_damage!=b.earth_damage||a.knocked_down_bonus!=0||a.condition!=b.condition||a.duration_seconds!=b.duration_seconds)return false;}return true;}
// Target position, relation, lifetime and base armor are trusted world inputs.
// Operates on copies: caller can preflight every foe before atomic world commit.
inline ShockwaveResult shockwave_target(const AoEPlan& plan,const ShockwaveTarget& target,double distance,unsigned level,uint32_t caster,uint64_t now,bool hostile,const EarthArmorSequence& armor_at_hit={},const EarthDamageResolver& damage_at_hit={},const EarthConditionReaction& condition_at_hit={}){
 ShockwaveResult result;result.after=target;const auto reviewed=caster_earth_aoe(937,plan.rank);
 if(!reviewed||plan.skill!=937||plan.count!=3||!caster||!std::isfinite(distance)||distance<0)return result;
 for(unsigned n=0;n<3;++n){const auto& a=plan.strikes[n];const auto& b=reviewed->strikes[n];if(a.radius!=b.radius||a.earth_damage!=b.earth_damage||a.knocked_down_bonus!=0||a.condition!=b.condition||a.duration_seconds!=b.duration_seconds)return result;}
 if(!hostile||distance>plan.strikes[0].radius){result.accepted=true;return result;}
 if(!target.actor.earth_armor)return result;const auto base=*target.actor.earth_armor;
 for(unsigned n=0;n<plan.count;++n){const auto& strike=plan.strikes[n];if(distance>strike.radius)continue;if(result.after.actor.health&&result.after.actor.health->current<=0)break;
  const auto current_base=armor_at_hit?armor_at_hit(result.hits.empty()?nullptr:&result.hits.back()):std::optional(base);
  if(!current_base)return ShockwaveResult{};
  result.after.actor.earth_armor=result.after.conditions.earth_armor(*current_base,now);
  const combat::Damage incoming{combat::Kind::earth,strike.earth_damage,level,937};
  auto hit=damage_at_hit?damage_at_hit(result.after.actor,incoming):combat::apply(result.after.actor,incoming,true);if(hit.reason!=combat::Reason::accepted)return ShockwaveResult{};result.hits.push_back(hit);if(result.after.actor.health&&result.after.actor.health->current<=0)break;
  const auto condition=strike.condition==Condition::weakness?combat::conditions::Kind::weakness:strike.condition==Condition::cracked_armor?combat::conditions::Kind::cracked_armor:combat::conditions::Kind::blind;
  if(!result.after.conditions.apply(condition,caster,now,uint64_t(strike.duration_seconds)*1000))return ShockwaveResult{};result.applied_conditions.push_back(condition);
  if(condition_at_hit&&!condition_at_hit(result.after,condition))return ShockwaveResult{};
 }
 result.after.actor.earth_armor=base;result.after.actor.effects=result.after.conditions.presentation(result.after.actor.effects,now);result.accepted=true;return result;
}
// Merge only this cast's intended condition applications onto current state.
// Damage/healing callbacks may have changed unrelated conditions since preview.
inline std::optional<combat::conditions::State> merge_earth_conditions(const AoEPlan& plan,const ShockwaveResult& result,
        combat::conditions::State current,unsigned caster,std::uint64_t now){
 if(!result.accepted||!caster)return {};
 if(result.applied_conditions.empty())return current;
 if(!valid_shockwave_plan(plan))return {};
 std::array<bool,10> seen{};
 for(const auto kind:result.applied_conditions){
  const auto index=static_cast<unsigned>(kind);if(index>=seen.size()||seen[index])return {};seen[index]=true;
  bool found=false;
  for(unsigned n=0;n<plan.count;++n){const auto& strike=plan.strikes[n];
   const auto intended=strike.condition==Condition::weakness?combat::conditions::Kind::weakness:strike.condition==Condition::cracked_armor?combat::conditions::Kind::cracked_armor:combat::conditions::Kind::blind;
   if(intended!=kind)continue;
   if(!current.apply(kind,caster,now,std::uint64_t(strike.duration_seconds)*1000))return {};
   found=true;break;
  }
  if(!found)return {};
 }
 return current;
}
}
