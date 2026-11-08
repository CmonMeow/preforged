#pragma once
#include "gw_energy_degeneration_hexes.h"
#include "gw_health_regeneration.h"
#include "gw_overcast_curve.h"
#include "gw_mind_wrack.h"
#include "gw_overcast_settlement.h"
namespace fw::gw::skills::effects {
struct EnergyHexOwnership {
 Scope caster,target;unsigned effect=0,caster_health_effect=0,skill=0,rank=0,level=0;
 Time applied=0,expires=0;
};
inline bool valid_energy_hex_owner(const EnergyHexOwnership& owner){
 const auto plan=energy_degeneration_hex(owner.skill,owner.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 return plan&&valid(owner.caster)&&valid(owner.target)&&owner.caster.instance==owner.target.instance&&owner.caster!=owner.target&&
  owner.caster.actor!=owner.target.actor&&owner.effect&&owner.level&&owner.level<=255&&
  bool(owner.caster_health_effect)==bool(plan->caster_health_degen)&&owner.expires>owner.applied&&owner.expires-owner.applied==plan->duration;
}
inline bool matches_energy_hex(const Registry& registry,const EnergyHexOwnership& owner){
 if(!valid_energy_hex_owner(owner)||!registry.matches(owner.target))return false;
 const auto entry=registry.lookup(owner.target,owner.effect);
 return entry&&entry->cause==owner.caster.actor&&entry->skill==owner.skill&&entry->rank==owner.rank&&entry->kind==Category::hex&&
  entry->applied==owner.applied&&entry->expires==owner.expires;
}
// Transaction-local copies of existing authorities,not another resource pool.
// The World owner must settle Energy to now and commit this complete result
// together with its lifetime-bound link record,or discard it entirely.
struct EnergyHexChange {
 Registry registry;regeneration::State health;overcast::CeilingCurve energy;
 std::map<Scope,regeneration::State> caster_health;
 EnergyHexOwnership ownership;std::vector<Entry> removed;
};
// World supplies exact-lifetime retirement evidence for death or departure.
// The missing timer is valid after death cleared that regeneration state;
// neither an arbitrary absent timer nor a future retirement is accepted.
using EnergyHexRetirements=std::map<Scope,Time>;
inline bool energy_hex_timer_retired(const regeneration::State& state,const EnergyHexOwnership& owner,
 Time now,const EnergyHexRetirements& retired){
 const auto death=retired.find(owner.caster);
 return death!=retired.end()&&state.matches(owner.caster)&&death->second>=owner.applied&&death->second<=now&&
  state.cursor()>=death->second&&state.cursor()<=now&&!state.lookup(owner.caster,owner.caster_health_effect);
}
inline bool energy_hex_sources_match(const regeneration::State& health,const overcast::CeilingCurve& energy,
 const std::map<Scope,regeneration::State>& casters,const EnergyHexOwnership& owner,Time now,const EnergyHexRetirements& retired={}){
 const auto plan=energy_degeneration_hex(owner.skill,owner.rank);if(!plan)return false;
 const auto timed=[&](const regeneration::State& state,Scope scope,unsigned effect,int pips){
  if(!state.matches(scope))return false;const auto source=state.lookup(scope,effect);
  if(!source)return now>=owner.expires&&state.cursor()>=owner.expires;
  return source->cause==owner.caster.actor&&source->skill==owner.skill&&source->pips==pips&&source->begins==owner.applied&&source->ends==owner.expires;
 };
 if(!timed(health,owner.target,owner.effect,-int(plan->target_health_degen)))return false;
 if(energy.has_energy_modifier(owner.target,owner.effect)){
  if(!energy.matches_energy_modifier(owner.target,owner.effect,owner.skill,-int(plan->energy_degen),owner.applied,owner.expires))return false;
 }else if(now<owner.expires)return false;
 if(owner.caster_health_effect){const auto source=casters.find(owner.caster);
  if(source==casters.end()||(!timed(source->second,owner.caster,owner.caster_health_effect,-int(plan->caster_health_degen))&&
     !energy_hex_timer_retired(source->second,owner,now,retired)))return false;}
 return true;
}
inline std::optional<EnergyHexChange> prepare_energy_hex(
 const Registry& registry,const regeneration::State& health,const overcast::CeilingCurve& energy,
 const std::map<Scope,regeneration::State>& caster_health,const EnergyHexOwnership& owner,
 std::optional<EnergyHexOwnership> previous={},const EnergyHexRetirements& retired={}){
 if(!valid_energy_hex_owner(owner)||!registry.matches(owner.target)||!health.matches(owner.target)||energy.cursor()!=owner.applied||
    registry.lookup(owner.target,owner.effect)||health.lookup(owner.target,owner.effect)||energy.has_energy_modifier(owner.target,owner.effect)||caster_health.size()>2)return {};
 for(const auto&[scope,state]:caster_health)if(!state.matches(scope))return {};
 if(previous&&(!matches_energy_hex(registry,*previous)||previous->target!=owner.target||previous->skill!=owner.skill||owner.applied<previous->applied))return {};
 const auto plan=*energy_degeneration_hex(owner.skill,owner.rank);
 EnergyHexChange result{registry,health,energy,caster_health,owner,{}};
 if(!result.energy.advance(owner.target,owner.applied))return {};
 if(previous&&!energy_hex_sources_match(health,energy,caster_health,*previous,owner.applied,retired))return {};
 const Entry entry{owner.effect,owner.caster.actor,owner.skill,owner.rank,Category::hex,owner.applied,owner.expires};
 const auto changed=result.registry.apply(owner.target,entry,Replace::same_skill);if(!changed.accepted)return {};
 if(changed.removed.size()!=unsigned(previous.has_value())||
    (previous&&changed.removed.front().instance!=previous->effect))return {};
 if(previous){
  if(!result.health.end(owner.target,previous->effect,owner.applied)&&owner.applied<previous->expires)return {};
  if(!result.energy.remove_energy_modifier(owner.target,previous->effect,owner.applied)&&owner.applied<previous->expires)return {};
  if(previous->caster_health_effect){
   const auto old=result.caster_health.find(previous->caster);
   if(old==result.caster_health.end()||(!old->second.end(previous->caster,previous->caster_health_effect,owner.applied)&&owner.applied<previous->expires&&
      !energy_hex_timer_retired(old->second,*previous,owner.applied,retired)))return {};
  }
 }
 if(!result.health.apply(owner.target,{owner.effect,owner.caster.actor,owner.skill,-int(plan.target_health_degen),owner.applied,owner.expires}))return {};
 if(!result.energy.apply_energy_modifier(owner.target,owner.effect,owner.skill,-int(plan.energy_degen),plan.duration,owner.applied))return {};
 if(plan.caster_health_degen){
  const auto source=result.caster_health.find(owner.caster);
  if(source==result.caster_health.end()||source->second.lookup(owner.caster,owner.caster_health_effect)||
     !source->second.apply(owner.caster,{owner.caster_health_effect,owner.caster.actor,owner.skill,-int(plan.caster_health_degen),owner.applied,owner.expires},false))return {};
 }
 result.removed=changed.removed;return result;
}
inline std::optional<EnergyHexChange> prepare_energy_hex_end(
 const Registry& registry,const regeneration::State& health,const overcast::CeilingCurve& energy,
 const std::map<Scope,regeneration::State>& caster_health,const EnergyHexOwnership& owner,Time now,const EnergyHexRetirements& retired={}){
 if(!matches_energy_hex(registry,owner)||!health.matches(owner.target)||now<owner.applied||energy.cursor()!=now||caster_health.size()>2)return {};
 for(const auto&[scope,state]:caster_health)if(!state.matches(scope))return {};
 EnergyHexChange result{registry,health,energy,caster_health,owner,{}};
 if(!result.energy.advance(owner.target,now)||!energy_hex_sources_match(health,energy,caster_health,owner,now,retired))return {};
 const auto removed=result.registry.remove(owner.target,owner.effect);if(!removed)return {};result.removed.push_back(*removed);
 if(!result.health.end(owner.target,owner.effect,now)&&now<owner.expires)return {};
 if(!result.energy.remove_energy_modifier(owner.target,owner.effect,now)&&now<owner.expires)return {};
 if(owner.caster_health_effect){
  const auto source=result.caster_health.find(owner.caster);
  if(source==result.caster_health.end()||(!source->second.end(owner.caster,owner.caster_health_effect,now)&&now<owner.expires&&
    !energy_hex_timer_retired(source->second,owner,now,retired)))return {};
 }
 return result;
}
struct EnergyHexZeroReaction {EnergyHexChange change;unsigned damage=0;};
struct EnergyHexZeroDamage {EnergyHexOwnership owner;unsigned damage=0;};
struct EnergyHexZeroBatch {
 Registry registry;regeneration::State health;overcast::CeilingCurve energy;
 std::map<Scope,regeneration::State> caster_health;
 std::vector<EnergyHexZeroDamage> damage;
};
// Only a boundary-stopped proposal can be converted to an observer event.
// A full-span result may have regained Energy after its first crossing.
inline std::optional<MindWrackTransition> energy_zero_observation(
 const overcast::Settlement& settled,Scope target,std::uint64_t event){
 if(!event||!settled.resources.health||!settled.first_zero||
    settled.curve.cursor()!=*settled.first_zero||!settled.first_zero_before_units||
    !*settled.first_zero_before_units||*settled.first_zero_before_units>1000||
    overcast::precise_current(settled)!=0)return {};
 auto curve=settled.curve;if(!curve.advance(target,curve.cursor()))return {};
 const MindWrackTransition observation{target,event,*settled.first_zero,
     {0,settled.resources.max_energy,0,static_cast<unsigned>(*settled.first_zero_before_units)},
     {0,settled.resources.max_energy,0,0}};
 if(!mind_wrack_zero_transition(observation))return {};
 return observation;
}
// Reuse the precise Energy transition contract shared by zero-Energy observers.
// World owns event creation, target survival and damage/protection settlement.
// Commit the ended effect before damage; replay then fails ownership lookup.
inline std::optional<EnergyHexZeroReaction> prepare_energy_hex_zero(
 const Registry& registry,const regeneration::State& health,const overcast::CeilingCurve& energy,
 const std::map<Scope,regeneration::State>& caster_health,const EnergyHexOwnership& owner,
 const MindWrackTransition& event,const EnergyHexRetirements& retired={}){
 if(!mind_wrack_zero_transition(event)||event.target!=owner.target||
    event.at<owner.applied||event.at>=owner.expires||health.cursor()!=event.at)return {};
 const auto plan=energy_degeneration_hex(owner.skill,owner.rank);if(!plan)return {};
 auto ended=prepare_energy_hex_end(registry,health,energy,caster_health,owner,event.at,retired);
 if(!ended)return {};
 return EnergyHexZeroReaction{std::move(*ended),plan->zero_damage};
}
// Stage every active zero-ending hex against the same observation. Publish
// this complete state before executing any damage intent: a lethal callback
// must not leave another eligible hex active. Intent order is bookkeeping,
// not a claim about retail simultaneous damage ordering.
inline std::optional<EnergyHexZeroBatch> prepare_energy_hex_zero_batch(
 const Registry& registry,const regeneration::State& health,const overcast::CeilingCurve& energy,
 const std::map<Scope,regeneration::State>& caster_health,std::vector<EnergyHexOwnership> owners,
 const MindWrackTransition& event,const EnergyHexRetirements& retired={}){
 if(!mind_wrack_zero_transition(event)||owners.empty()||owners.size()>2)return {};
 std::sort(owners.begin(),owners.end(),[](const auto& a,const auto& b){return a.effect<b.effect;});
 if(owners.size()==2&&owners[0].effect==owners[1].effect)return {};
 std::size_t eligible=0;
 for(const auto& entry:registry.active(event.target,event.at))if(entry.skill==125||entry.skill==140){
  ++eligible;
  if(std::none_of(owners.begin(),owners.end(),[&](const auto& owner){return owner.effect==entry.instance;}))return {};
 }
 if(eligible!=owners.size())return {};
 EnergyHexZeroBatch result{registry,health,energy,caster_health,{}};
 for(const auto& owner:owners){
  auto ended=prepare_energy_hex_zero(result.registry,result.health,result.energy,result.caster_health,owner,event,retired);
  if(!ended)return {};
  result.registry=std::move(ended->change.registry);result.health=std::move(ended->change.health);
  result.energy=std::move(ended->change.energy);result.caster_health=std::move(ended->change.caster_health);
  result.damage.push_back({owner,ended->damage});
 }
 return result;
}
struct EnergyZeroObservers {
 EnergyHexZeroBatch hexes;MindWrackEvents events;
 std::optional<MindWrackReaction> mind_wrack;
};
// One event ledger spans all zero observers and survives same-lifetime renewal.
// The dispatcher commits the returned state before invoking any damage callback.
inline std::optional<EnergyZeroObservers> prepare_energy_zero_observers(
 const Registry& registry,const regeneration::State& health,const overcast::CeilingCurve& energy,
 const std::map<Scope,regeneration::State>& caster_health,const std::vector<EnergyHexOwnership>& owners,
 std::optional<MindWrackOwnership> mind_owner,const MindWrackEvents& events,const MindWrackTransition& event,const EnergyHexRetirements& retired={}){
 if(!mind_wrack_zero_transition(event)||!registry.matches(event.target)||!health.matches(event.target)||
    health.cursor()!=event.at||energy.cursor()!=event.at)return {};
 auto curve=energy;if(!curve.advance(event.target,event.at))return {};
 unsigned degeneration_count=0,mind_count=0;
 for(const auto& entry:registry.active(event.target,event.at)){
  degeneration_count+=entry.skill==125||entry.skill==140;
  if(entry.skill==49||entry.skill==2734){++mind_count;if(!mind_owner||mind_owner->effect!=entry.instance)return {};}
 }
 if(degeneration_count!=owners.size()||mind_count!=unsigned(mind_owner.has_value())||(!degeneration_count&&!mind_count))return {};
 auto accepted=events;if(!accepted.accept(event))return {};
 EnergyHexZeroBatch batch{registry,health,energy,caster_health,{}};
 if(!owners.empty()){
  auto staged=prepare_energy_hex_zero_batch(registry,health,energy,caster_health,owners,event,retired);
  if(!staged)return {};batch=std::move(*staged);
 }
 std::optional<MindWrackReaction> reaction;
 if(mind_owner){
  reaction=mind_wrack_energy_reaction(batch.registry,*mind_owner,event);
  if(!reaction||!reaction->ends)return {};
  batch.registry=reaction->registry;
 }
 return EnergyZeroObservers{std::move(batch),std::move(accepted),std::move(reaction)};
}
}
