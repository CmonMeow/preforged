#pragma once
#include "gw_condition_transfer.h"
#include "gw_effect_registry.h"
#include <span>
#include <set>
namespace fw::gw::skills::effects {
struct MartyrPlan {
 unsigned energy=5;Time activation=1000,aftercast=750,recharge=10000;float range=5000;
 auto operator<=>(const MartyrPlan&)const=default;
};
inline std::optional<MartyrPlan> martyr(){
 const auto* r=find_skill(298);
 if(!r||r->profession!=3||r->attribute!=51||r->type!=5||r->special!=4||r->energy_cost()!=5||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=10||r->caster_body_resource!=2077||r->target_body_resource!=524)return {};
 return MartyrPlan{};
}
struct MartyrAlly {Scope scope;float distance;bool alive,allied;combat::conditions::State conditions;};
struct MartyrTransfer {Scope scope;combat::conditions::State conditions;std::vector<combat::conditions::Kind> removed;};
struct MartyrProposal {Scope caster;combat::conditions::State conditions;std::vector<MartyrTransfer> allies;};
// World must settle and prove these exact lifetimes/allegiances/positions before
// committing. Copies keep one recipient failure from partially cleansing allies.
inline std::optional<MartyrProposal> prepare_martyr(const MartyrPlan& plan,Scope caster,
 const combat::conditions::State& own,std::span<const MartyrAlly> observed,Time now){
 const auto canonical=martyr();const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(caster)||observed.size()>1024)return {};
 std::set<unsigned> actors;std::vector<const MartyrAlly*> selected;
 for(const auto& ally:observed){
  if(!valid(ally.scope)||ally.scope.instance!=caster.instance||!actors.insert(ally.scope.actor).second||!std::isfinite(ally.distance)||ally.distance<0)return {};
  if(ally.scope.actor==caster.actor){if(ally.scope!=caster)return {};continue;}
  if(ally.alive&&ally.allied&&ally.distance<=plan.range)selected.push_back(&ally);
 }
 // Explicit deterministic host order;retail tie order is not established.
 std::sort(selected.begin(),selected.end(),[](auto a,auto b){return a->scope.actor<b->scope.actor;});
 MartyrProposal result{caster,own,{}};
 const auto policy=*condition_transfer(154,0);
 // Plague Touch's reviewed copier preserves remaining timers and longest
 // existing recipient duration;Martyr selects all10rather than Touch's count.
 for(const auto* ally:selected){
  auto remaining=ally->conditions;std::vector<combat::conditions::Kind> removed;
  for(unsigned i=0;i<10&&remaining.newest(now);++i){
   const auto next=transfer_conditions(policy,remaining,result.conditions,caster.actor,now);if(!next)return {};
   remaining=next->source;result.conditions=next->target;removed.insert(removed.end(),next->kinds.begin(),next->kinds.end());
  }
  result.allies.push_back({ally->scope,std::move(remaining),std::move(removed)});
 }
 return result;
}
}



