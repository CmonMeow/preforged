#pragma once
#include "gw_attack_outcomes.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
#include <set>
#include <span>
#include <vector>
namespace fw::gw::combat::weapon {
struct EarthShakerPlan {
 unsigned skill=354,rank=0,adrenaline=200,required_type=15;
 std::uint64_t knockdown_ms=2000;
 double adjacent=166.,failure_chance=.5;
 bool operator==(const EarthShakerPlan&)const=default;
};
inline std::optional<EarthShakerPlan> earth_shaker(unsigned rank){
 const auto* r=skills::find_skill(354);
 if(rank>20||!r||!r->elite()||r->type!=14||r->attribute!=19||r->weapon_requirement!=16||
    r->energy_cost()!=0||r->adrenaline!=200||r->activation!=0||r->aftercast!=0||r->recharge!=0||
    r->scale0||r->scale15||r->bonus0||r->bonus15||r->duration0!=2||r->duration15!=2||
    r->caster_body_resource!=2077||r->target_body_resource!=606)return {};
 return EarthShakerPlan{354,rank,200,15,2000,166.,rank<=4?.5:0.};
}
// Draw once from the authoritative RNG, independently of accuracy/block. A
// failure spends the attack's resources but never becomes a qualified hit.
inline std::optional<bool> earth_shaker_fails(const EarthShakerPlan& p,double roll){
 const auto canonical=earth_shaker(p.rank);
 if(!canonical||*canonical!=p||!std::isfinite(roll)||roll<0||roll>=1)return {};
 return roll<p.failure_chance;
}
struct EarthShakerRecipient {
 skills::effects::Scope scope;double x=0,y=0;bool eligible=false;
};
// Selection only; World supplies settled positions and authoritative hostile,
// alive, same-layer eligibility. Knockdown immunity and duration modifiers are
// resolved by the existing control path, not inferred from attack immunity.
// Only primary receives weapon damage; secondary recipients are NOT attacks.
inline std::optional<std::vector<skills::effects::Scope>> earth_shaker_recipients(
 const EarthShakerPlan& p,skills::effects::Scope source,skills::effects::Scope primary,
 AttackOutcome outcome,bool skill_failed,std::span<const EarthShakerRecipient> candidates){
 const auto canonical=earth_shaker(p.rank);
 const auto valid_scope=[](skills::effects::Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=p||!valid_scope(source)||!valid_scope(primary)||source.instance!=primary.instance||
    source.actor==primary.actor||!valid_attack_outcome(outcome)||candidates.size()>4096)return {};
 const EarthShakerRecipient* center=nullptr;std::set<unsigned> seen;
 for(const auto& c:candidates){
  if(!valid_scope(c.scope)||c.scope.instance!=source.instance||!seen.insert(c.scope.actor).second||
     !std::isfinite(c.x)||!std::isfinite(c.y)||(c.scope.actor==source.actor&&c.scope!=source))return {};
  if(c.scope==primary)center=&c;
 }
 if(!center)return {};
 std::vector<skills::effects::Scope> selected;
 if(skill_failed||outcome!=AttackOutcome::hit)return selected;
 // Primary can have died to the weapon damage. Retain its authoritative
 // contact position as the center, without applying control to its corpse.
 if(center->eligible)selected.push_back(primary);
 for(const auto& c:candidates){
  if(c.scope.actor==source.actor||c.scope==primary||!c.eligible||
     std::hypot(c.x-center->x,c.y-center->y)>p.adjacent)continue;
  selected.push_back(c.scope);
 }
 return selected;
}
}
