#pragma once
#include "gw_effect_registry.h"
#include "gw_aura_restoration.h"

namespace fw::gw::skills::effects {
struct SuccorPlan {
 unsigned health_pips=1,energy_pips=1,upkeep=1,spell_loss=1,energy=5;
 Time activation=1000,aftercast=750,recharge=10000;
 auto operator<=>(const SuccorPlan&)const=default;
};
inline std::optional<SuccorPlan> succor(){
 const auto* r=find_skill(308);
 if(!r||r->profession!=3||r->attribute!=51||r->type!=6||r->special||r->energy_cost()!=5||r->health_sacrifice||r->adrenaline||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=10||r->scale0!=1||r->scale15!=1||r->bonus0!=1||r->bonus15!=1||r->duration0!=131072||r->duration15!=131072||r->caster_body_resource!=547||r->target_body_resource!=548)return {};
 return SuccorPlan{};
}
inline bool succor_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
// The World must retain both generations: Entry::cause alone cannot distinguish
// a maintainer who left from a new actor reusing its numeric ID. This lease is
// server-owned; registry removal remains the authority for whether it is active.
struct SuccorLease {Scope caster,target;unsigned effect=0;Time applied=0;auto operator<=>(const SuccorLease&)const=default;};
inline bool valid_succor_lease(const SuccorLease& l){
 return succor().has_value()&&succor_scope(l.caster)&&succor_scope(l.target)&&l.caster.instance==l.target.instance&&l.caster.actor!=l.target.actor&&l.effect;
}
inline std::optional<SuccorPlan> succor_contribution(const SuccorLease& l,Scope live_caster,Scope live_target,const Registry& registry,Time now){
 if(!valid_succor_lease(l)||l.caster!=live_caster||l.target!=live_target||!registry.matches(l.target)||now<l.applied)return {};
 const auto e=registry.lookup(l.target,l.effect);
 if(!e||e->skill!=308||e->rank||e->cause!=l.caster.actor||e->kind!=Category::enchantment||e->applied!=l.applied||e->expires)return {};
 return succor();
}
struct SuccorApplication {Registry registry;SuccorLease lease;std::vector<Entry> removed;};
// Detached copy-only registry producer. Allied/live targeting, Energy payment,
// regeneration and maintained ownership must preflight before World commits it.
inline std::optional<SuccorApplication> prepare_succor(Scope caster,Scope target,unsigned effect,Time now,const Registry& registry){
 const SuccorLease lease{caster,target,effect,now};if(!valid_succor_lease(lease)||!registry.matches(target))return {};
 SuccorApplication result{registry,lease,{}};
 const auto applied=result.registry.apply(target,{effect,caster.actor,308,0,Category::enchantment,now,{}},Replace::same_skill);
 if(!applied.accepted)return {};result.removed=applied.removed;return result;
}
// Feed only accepted spell completions, in the target lifetime's authoritative
// sequence. Failed/interrupted starts are not completions. A duplicate cannot
// debit twice. The callback must atomically perform one non-cost Energy loss
// on this exact maintainer and publish reactions under the World lock. It must
// return false without mutation on failure; caller may then retry this event.
// One consumer belongs to one lease; replacement creates a fresh consumer.
class SuccorSpellConsumer {
 SuccorLease lease_;std::uint64_t sequence_=0;Time cursor_=0;
public:
 explicit SuccorSpellConsumer(SuccorLease lease):lease_(lease),cursor_(lease.applied){}
 std::uint64_t sequence()const{return sequence_;}
 template<class Lose> std::optional<unsigned> completed(Scope caster,Scope target,const Registry& registry,unsigned skill,std::uint64_t sequence,Time now,Lose&& lose){
  if(!valid_succor_lease(lease_)||caster!=lease_.caster||target!=lease_.target||!registry.matches(target)||!sequence||sequence<=sequence_||now<cursor_)return {};
  const auto* spell=find_skill(skill);if(!skill||!spell)return {};
  unsigned loss=0;
  if(const auto plan=succor_contribution(lease_,caster,target,registry,now);plan&&is_spell(*spell)){
   loss=plan->spell_loss;if(!lose(caster,loss))return {};
  }
  sequence_=sequence;cursor_=now;return loss;
 }
};
}
