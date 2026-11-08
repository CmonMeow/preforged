#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct ThunderclapPlan {
 unsigned rank=0;double damage=10;unsigned penetration=25;
 std::uint64_t condition_ms=5000;float range=1248,radius=166;
 auto operator<=>(const ThunderclapPlan&)const=default;
};
inline std::optional<ThunderclapPlan> thunderclap(unsigned rank){
 const auto* row=find_skill(228);
 if(rank>20||!row||row->type!=5||row->attribute!=8||row->scale0!=10||row->scale15!=50||
    row->bonus0!=5||row->bonus15!=20||row->encoded_energy!=10||row->recharge!=8)return {};
 return ThunderclapPlan{rank,damage_scale(10,50,rank),25,std::uint64_t(interpolate(5,20,rank))*1000};
}
struct ThunderclapResult {
 bool accepted=false;combat::Actor actor;combat::conditions::State conditions;
 combat::Hit hit;std::vector<combat::conditions::Kind> applied_conditions;
};
// Pure recipient preflight only. World must own the mixed recipient set,
// condition reactions, action interruption/protection and publication order.
// The World lightning-area path owns those operations; this helper supplies
// independent rank and recipient-policy checks.
inline ThunderclapResult thunderclap_target(const ThunderclapPlan& plan,combat::Actor actor,
 combat::conditions::State conditions,unsigned level,std::uint32_t caster,std::uint64_t now,
 bool allows_conditions=true){
 const auto canonical=thunderclap(plan.rank);
 if(!canonical||*canonical!=plan||!caster||!actor.lightning_armor)return {};
 actor.lightning_armor=conditions.armor(*actor.lightning_armor,now);
 const auto hit=combat::apply(actor,{combat::Kind::lightning,plan.damage,level,228,plan.penetration},true);
 if(hit.reason!=combat::Reason::accepted)return {};
 ThunderclapResult result{true,actor,conditions,hit,{}};
 if(hit.killed||!allows_conditions)return result;
 // Historical skill notes: damage precedes both conditions; Weakness covers
 // Cracked Armor. Reversed insertion changes which condition is removed first.
 for(auto kind:{combat::conditions::Kind::cracked_armor,combat::conditions::Kind::weakness}){
  if(!result.conditions.apply(kind,caster,now,plan.condition_ms))return {};
  result.applied_conditions.push_back(kind);
 }
 return result;
}
}
