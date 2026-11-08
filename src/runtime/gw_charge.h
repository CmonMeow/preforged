#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
namespace fw::gw::skills::effects {
struct ChargePlan {
 unsigned rank=0,energy=5;Time duration=5000,recharge=20000;float range=1012,multiplier=1.33f;
 auto operator<=>(const ChargePlan&)const=default;
};
inline std::optional<ChargePlan> charge(unsigned rank){
 const auto* row=find_skill(364);
 if(rank>20||!row||row->profession!=1||row->attribute!=21||row->type!=15||row->special!=4||row->energy_cost()!=5||row->adrenaline||row->health_sacrifice||
    row->activation!=0||row->aftercast!=0||row->recharge!=20||row->duration0!=5||row->duration15!=13||row->scale0!=33||row->scale15!=33||row->bonus0||row->bonus15||
    row->caster_body_resource!=622||row->target_body_resource!=2077)return {};
 return ChargePlan{rank,5,1000ull*interpolate(5,13,rank)};
}
inline std::optional<Entry> prepare_charge(const ChargePlan& plan,Scope source,Scope target,unsigned effect,Time now){
 const auto canonical=charge(plan.rank);const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(source)||!valid(target)||source.instance!=target.instance||!effect||now>UINT64_MAX-plan.duration)return {};
 return Entry{effect,source.actor,364,plan.rank,Category::other,now,now+plan.duration};
}
// Pure proposal only. The World must admit ALL allies in earshot (not merely
// party members),settle movement,commit the cleanse and publish its reactions.
struct ChargeCleanse {combat::conditions::State conditions;std::optional<combat::conditions::Entry> removed;};
inline std::optional<ChargeCleanse> charge_cleanse(const ChargePlan& plan,const combat::conditions::State& conditions,Time now){
 const auto canonical=charge(plan.rank);if(!canonical||*canonical!=plan)return {};
 ChargeCleanse proposed{conditions,{}};
 if(conditions.has(combat::conditions::Kind::crippled,now)){
  proposed.removed=conditions.lookup(combat::conditions::Kind::crippled);
  if(!proposed.conditions.remove(combat::conditions::Kind::crippled))return {};
 }
 return proposed;
}
}
