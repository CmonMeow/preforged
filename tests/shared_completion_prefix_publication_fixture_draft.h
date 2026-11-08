#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
inline void publication_check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Shared completion publication line "+std::to_string(at.line()));
}
struct CompletionPrefixPublicationFixture {
 std::unique_ptr<world::Instance> store=std::make_unique<world::Instance>();world::Instance& w=*store;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope owner,hexer;
 casting::Runtime runtime{1};casting::Hooks h;Slots bar{};Policy policy;attributes::Ranks ranks{};
 unsigned skill,initial,paid=0,payments=0,completed=0,effects_committed=0,finalized=0,copied=0,aborted=0;
 unsigned health_at_finalize=0,health_at_copy=0;bool inject_refusal=false;std::uint64_t clock=100;
 std::vector<Packet> early;const effects::VigorousSpiritPlan vigor=*effects::vigorous_spirit(0);
 const items::weapon_passives::EnchantingEquipment equipment=*items::weapon_passives::equipped_enchanting(inventory::Profile{},1);
 CompletionPrefixPublicationFixture(unsigned id=1375,unsigned health=100,bool spiteful=false):skill(id),initial(health){
  w.join(1,"Publish caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{health,1000,40,40},100);owner=*w.effect_scope(1);
  if(spiteful){w.join(2,"Publish hexer",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);hexer=*w.effect_scope(2);publication_check(w.spiteful_spirit(hexer,owner,*effects::spiteful_spirit(12),20,100));}
  publication_check(w.complete_vigorous_spirit(owner,owner,vigor,0,100));bar[0]={static_cast<std::uint16_t>(skill),0};publication_check(w.configure_adrenaline(owner,bar,100));
  policy.primary=skill==115?4:6;policy.secondary=skill==115?6:3;policy.campaigns=255;policy.learned[skill]=1;
  h.resources=[&](auto actor){return w.resources(actor);};h.events=[&](const auto& batch){w.skill_event(1,batch,clock);};
  h.target_scope=[&](auto actor){return w.effect_scope(actor);};h.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
  h.consume_energy_at=[&](auto cost,auto at){++payments;paid+=cost;return w.consume_energy(1,cost,owner,at);};
  h.effect_snapshot=[&](auto at){return w.status_snapshot(owner,at);};h.before_spell_effect=[&](auto id,auto event,auto at){return w.before_spell_effect(owner,id,event,at);};
  h.enchantment_duration_ready=[&](auto id,auto base,auto at){return bool(quote(id,base,at));};
  h.simple_enchantment_completed=[&](auto target,auto id,auto rank,unsigned,auto event,auto at)->casting::EffectCompletion{
   publication_check(target==owner&&id==1375);auto plan=effects::stoneflesh_aura(rank);auto duration=plan?quote(id,plan->duration,at):std::nullopt;
   casting::EffectCompletion result;std::uint64_t completed_at=0;
   result.accepted=duration&&w.stoneflesh_aura_enchantment(owner,rank,at,event,&*duration,&completed_at,&result.publication);
   if(result.accepted){++effects_committed;result.effect_snapshot_at=completed_at;}return result;
  };
  // Orison's original World heal is a nondeferred producer. Only the reaction
  // publication is asserted here; this test does not rewrite its legacy output.
  h.heal=[&](auto actor,auto amount,float range,bool self){const auto target=w.effect_scope(actor);const bool accepted=target&&w.heal_target_scoped(owner,*target,amount,range,self,clock);if(accepted)++effects_committed;return accepted;};
  h.restore_energy=[&](auto amount){return w.restore_energy(1,amount,owner);};
  h.apply_health_regeneration=[&](auto actor,const auto& plan,auto at,auto ends,float range,bool self){const bool accepted=w.apply_health_regeneration(owner,actor,plan,at,ends,range,self);if(accepted)++effects_committed;return accepted;};
  h.health_sacrifice=[&](auto id,auto at){return w.health_sacrifice(owner,id,at);};
  h.prepare_completion_publication=[&](auto source,auto at){return w.prepare_completion_publication(source,at);};
  h.completed_action=[&](auto id,auto at)->casting::ActionCompletion{
   ++completed;casting::ActionCompletion result;result.accepted=w.completed_skill_action(owner,id,at,nullptr,&result.publication);
   if(inject_refusal)result.accepted=false;return result;
  };
  h.begin_action=[&](auto slot,auto at){publication_check(w.accepted_skill_start(owner,slot,at));};
  h.prepare_skill_use=[&](auto slot,auto event,auto at){return w.prepare_diversion_skill_use(owner,slot,event,at);};
  h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){
   ++finalized;health_at_finalize=w.resources(1)->health;collect_early();return w.finalize_diversion_skill_use(owner,event,at,base,existing,instant);
  };
  h.abort_skill_use=[&](auto event){++aborted;return w.abort_diversion_skill_use(owner,event);};
  h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(owner,event);};
  h.completed_skill_use=[&](const effects::CooldownCompletion& receipt){
   ++copied;publication_check(receipt.stamp.owner==owner&&receipt.slot==Slot{static_cast<std::uint16_t>(skill),0});health_at_copy=w.resources(1)->health;collect_early();return true;
  };
  box->drain();
 }
 void collect_early(){auto packets=box->drain();const auto heal=particles::health_change_2022(1,1,vigor.healing,1000);publication_check(std::find(packets.begin(),packets.end(),heal)==packets.end());early.insert(early.end(),packets.begin(),packets.end());}
 std::optional<effects::EnchantmentDuration> quote(unsigned id,std::uint64_t base,std::uint64_t at){const auto active=w.status_snapshot(owner,at);return active?effects::enchanting_duration(id,base,owner,equipment,*active,at):std::nullopt;}
 void start(){clock=1000;publication_check(runtime.start(clock,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);publication_check(payments==1&&paid==find_skill(skill)->energy_cost());box->drain();}
 void update(std::uint64_t at){clock=at;runtime.update(at,h);}
 std::vector<Packet> finish(std::uint64_t due){start();update(due-1);publication_check(!effects_committed&&!completed);box->drain();update(due);auto tail=box->drain();auto result=early;result.insert(result.end(),tail.begin(),tail.end());return result;}
};
