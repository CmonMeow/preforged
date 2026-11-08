#pragma once
#include "gw_effect_registry.h"
#include "gw_combat.h"
#include "gw_skill_effect_plan.h"
#include <limits>
namespace fw::gw::skills::effects {
struct ShieldAbsorptionPlan {unsigned rank;Time duration;unsigned step=5;};
inline std::optional<ShieldAbsorptionPlan> shield_absorption(unsigned rank){
 const auto* row=find_skill(1399);
 if(rank>20||!row||row->type!=6||row->profession!=3||row->attribute!=15||
    row->energy_cost()!=5||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=10||
    row->scale0!=5||row->scale15!=5||row->duration0!=3||row->duration15!=7||
    row->caster_body_resource!=2077||row->target_body_resource!=1650)return {};
 return ShieldAbsorptionPlan{rank,Time(interpolate(3,7,rank))*1000,5};
}
// Transaction-local hit counter. Damage preflight never advances it. A caller
// commits only an accepted damage receipt, then publishes that transaction.
// This authority alone is not a production binding or a damage classification.
class ShieldAbsorption {
 Scope owner_;std::optional<Entry> effect_;std::uint64_t hits_=0,last_event_=0,application_=0;Time last_time_=0;
 static bool same(const Entry& a,const Entry& b){return a.skill==b.skill&&a.kind==b.kind&&a.instance==b.instance&&a.cause==b.cause&&a.applied==b.applied&&a.rank==b.rank&&a.expires==b.expires;}
 static bool valid(const Entry& e,Time now){return e.skill==1399&&e.kind==Category::enchantment&&e.instance&&e.cause&&shield_absorption(e.rank)&&e.applied<=now&&e.expires&&now<*e.expires;}
public:
 struct Prepared {Entry effect;std::uint64_t event=0,prior_hits=0,application=0;Time at=0;unsigned reduction=0;};
 explicit ShieldAbsorption(Scope owner):owner_(owner){}
 std::uint64_t last_event()const{return last_event_;}
 static combat::CumulativeReductionReceipt receipt(const Prepared& p){return {1399,p.effect.instance,p.reduction,p.application,p.event,p.prior_hits,p.at};}
 bool commit_receipt(const Registry& registry,Scope target,const combat::CumulativeReductionReceipt& stamp,Time now){
  const auto p=prepare(registry,target,stamp.event,now);
  return p&&receipt(*p)==stamp&&commit(registry,target,*p,now);
 }

 bool same_state(const ShieldAbsorption& other)const{
  return owner_==other.owner_&&hits_==other.hits_&&last_event_==other.last_event_&&application_==other.application_&&last_time_==other.last_time_&&
   bool(effect_)==bool(other.effect_)&&(!effect_||same(*effect_,*other.effect_));
 }

 std::optional<Prepared> prepare(const Registry& registry,Scope target,std::uint64_t event,Time now)const{
  if(target!=owner_||!registry.matches(target)||!event||event<=last_event_||now<last_time_)return {};
  const auto e=registry.newest_skill(target,1399,now);if(!e||!valid(*e,now))return {};
  const auto application=registry.entry_sequence(target,e->instance);if(!application)return {};
  const auto prior=effect_&&application_==*application&&same(*effect_,*e)?hits_:0;
  if(prior==UINT64_MAX)return {};
  const unsigned reduction=prior>=UINT_MAX/5?UINT_MAX:unsigned(prior+1)*5;
  return Prepared{*e,event,prior,*application,now,reduction};
 }
 bool commit(const Registry& registry,Scope target,const Prepared& receipt,Time now){
  if(now!=receipt.at)return false;
  const auto current=prepare(registry,target,receipt.event,now);
  if(!current||current->application!=receipt.application||!same(current->effect,receipt.effect)||current->prior_hits!=receipt.prior_hits||current->reduction!=receipt.reduction)return false;
  effect_=receipt.effect;application_=receipt.application;hits_=receipt.prior_hits+1;last_event_=receipt.event;last_time_=now;return true;
 }
};
}
