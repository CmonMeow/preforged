#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Energy hex line "+std::to_string(at.line()));
}
Definition profile(unsigned current=10){Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
 d.relation=Relation::hostile_to_players;d.spawn_health=Health{1000,1000};d.damageable=d.conditions_allowed=true;
 d.spawn_energy=Energy{current,20};d.energy_pips=0;return d;}
int main()try{
 const Lifetime life{1,2,3,146,100};const skills::effects::Scope source{1,1,1};
 using Removed=LiveActor::HexRemovalResult;
 {
  LiveActor actor(profile(),life);
  check(actor.apply_movement_hex(life,*skills::effects::movement_hex(76,0),1,77,1000));
  const auto lease=actor.prepare_hex_removal(life,77,1000,true);check(bool(lease));
  check(actor.apply_movement_hex(life,*skills::effects::movement_hex(54,0),1,78,1000));
  check(!actor.remove_mesmer_hex(life,77,1000)); // It is no longer newest.
  check(actor.commit_hex_removal(*lease,1000)==Removed::removed);
  check(!actor.hex_effect(life,77)&&actor.hex_effect(life,78));
  check(actor.commit_hex_removal(*lease,1000)==Removed::already_absent);
  check(actor.apply_movement_hex(life,*skills::effects::movement_hex(76,0),1,77,1000));
  const auto renewed=actor.prepare_hex_removal(life,77,1000,true);check(bool(renewed));
  check(actor.apply_movement_hex(life,*skills::effects::movement_hex(76,0),1,77,1000));
  check(actor.commit_hex_removal(*renewed,1000)==Removed::replaced);
  check(actor.hex_effect(life,77)&&actor.hex_effect(life,78));
  auto forged=*renewed;forged.entry.skill=54;
  // Use a fresh sequence so metadata validation, not renewal detection, rejects it.
  forged.sequence=actor.prepare_hex_removal(life,77,1000,true)->sequence;
  check(!actor.commit_hex_removal(forged,1000));
  auto stale=*renewed;++stale.expected.actor_generation;check(!actor.commit_hex_removal(stale,1000));
  check(!actor.commit_hex_removal(*renewed,999));
 }
 {
  LiveActor actor(profile(),life);
  check(actor.apply_movement_hex(life,*skills::effects::movement_hex(210,0),1,77,1000));
  check(!actor.prepare_hex_removal(life,77,1000,true));
  const auto lease=actor.prepare_hex_removal(life,77,1000);check(bool(lease));
  check(actor.stage_shield_absorption(life,12,1,78,1000));
  check(!actor.prepare_hex_removal(life,78,1000));
  check(actor.apply_damage(life,{Kind::armor_ignoring,50,20,27},1000).damage==45);
  const auto health=actor.actor(life)->health->current;
  check(actor.commit_hex_removal(*lease,1000)==Removed::removed);
  check(actor.actor(life)->health->current==health&&actor.movement_multiplier(life,1000)==1.f);
  check(actor.apply_damage(life,{Kind::armor_ignoring,50,20,27},1001).damage==40);
 }
 {
  LiveActor actor(profile(),life);const auto plan=*skills::effects::movement_hex(54,0);
  check(actor.apply_movement_hex(life,plan,1,77,1000));
  const auto lease=actor.prepare_hex_removal(life,77,1000,true);check(bool(lease));
  check(!actor.commit_hex_removal(*lease,1001));
  check(bool(actor.hex_effect(life,77)));
  check(actor.advance_conditions(life,1000+plan.duration).has_value());
  actor.refresh_hexes(life,1000+plan.duration);
  check(actor.commit_hex_removal(*lease,1000+plan.duration)==Removed::already_absent);
 }
 for(unsigned skill:{125u,140u}){
  LiveActor actor(profile(),life);const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
  check(actor.apply_energy_degeneration_hex(life,source,plan,77,1000).has_value());
  const auto forecast=actor.forecast_energy_zero(life,4000);check(forecast&&!forecast->first_zero&&forecast->resources.energy==9);
  check(actor.energy(life)->current==10&&actor.actor(life)->health->current==1000);
  check(actor.advance_conditions(life,4000).has_value());
  check(actor.energy(life)->current==9&&actor.actor(life)->health->current==(skill==125?976:1000));
  check(actor.remove_hex_effect(life,77,4000)&&actor.advance_conditions(life,7000).has_value());
  check(actor.energy(life)->current==9&&actor.actor(life)->health->current==(skill==125?976:1000));
 }
 for(unsigned mode=0;mode<4;++mode){auto d=profile();if(mode==0)d.spawn_energy.reset();if(mode<=1)d.energy_pips.reset();
  if(mode==2)d.relation=Relation::neutral;
  LiveActor actor(d,life);auto selected=life;if(mode==3)++selected.actor_generation;
  check(!actor.apply_energy_degeneration_hex(selected,source,*skills::effects::energy_degeneration_hex(125,12),77,1000));
  check(!actor.hex_effect(life,77));
 }
 {
  LiveActor actor(profile(1),life);check(actor.apply_energy_degeneration_hex(life,source,*skills::effects::energy_degeneration_hex(125,12),77,1000).has_value());
  const auto forecast=actor.forecast_energy_zero(life,10000);check(forecast&&forecast->first_zero==4000&&forecast->curve.cursor()==4000);
  check(!actor.advance_conditions(life,10000)&&actor.actor(life)->health->current==1000&&actor.energy(life)->current==1);
  check(actor.advance_conditions(life,4000).has_value());
  const auto event=actor.pending_energy_zero(life,99);check(event&&event->at==4000&&skills::effects::mind_wrack_zero_transition(*event));
  check(actor.actor(life)->health->current==976&&actor.energy(life)->current==0);
  check(!actor.advance_conditions(life,10000)&&actor.actor(life)->health->current==976);
  check(actor.pending_energy_zero(life,99)->before==event->before);
 }
 {
  LiveActor actor(profile(),life);check(actor.apply_energy_degeneration_hex(life,source,*skills::effects::energy_degeneration_hex(125,12),77,1000).has_value());
  check(actor.advance_conditions(life,2000).has_value());const skills::effects::Scope replacement{1,2,1};
  const auto renewed=actor.apply_energy_degeneration_hex(life,replacement,*skills::effects::energy_degeneration_hex(125,0),78,2000);
  check(renewed&&renewed->removed.size()==1&&!actor.hex_effect(life,77)&&actor.hex_effect(life,78)->cause==2);
  check(actor.advance_conditions(life,9000).has_value());
  check(actor.actor(life)->health->current==972&&actor.energy(life)->current==8);
 }
 for(bool mind:{false,true}){
  LiveActor actor(profile(1),life);std::vector<skills::effects::EnergyHexOwnership> owners;
  for(unsigned skill:{125u,140u}){
   const unsigned effect=skill==125?77:78;const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
   check(actor.apply_energy_degeneration_hex(life,source,plan,effect,1000).has_value());
   owners.push_back({source,actor.hex_scope(),effect,skill==140?10u:0u,skill,12,20,1000,1000+plan.duration});
  }
  std::optional<skills::effects::MindWrackOwnership> mind_owner;
  if(mind){check(actor.apply_mind_wrack(life,source,*skills::effects::mind_wrack(12),80,1000).has_value());mind_owner=skills::effects::MindWrackOwnership{actor.hex_scope(),source,80};}
  check(actor.advance_conditions(life,2500).has_value());const auto event=*actor.pending_energy_zero(life,99);
  check(!actor.remove_hex_effect(life,77,2500)&&!actor.remove_hex_effect(life,78,2500));
  if(mind)check(!actor.remove_mesmer_hex(life,80,2500));
  check(actor.hex_effect(life,77)&&actor.hex_effect(life,78));
  check(!actor.gain_energy(life,1)&&!actor.lose_energy(life,0)&&!actor.drain_energy(life,1)&&!actor.advance_energy(life,2500));
  check(actor.pending_energy_zero(life,99)->before==event.before&&actor.energy(life)->current==0);
  check(!actor.commit_energy_hex_zero(life,{owners.front()},{},event));
  check(actor.hex_effect(life,77)&&actor.hex_effect(life,78)&&actor.pending_energy_zero(life,99));
  auto forged=event;++forged.at;check(!actor.commit_energy_hex_zero(life,owners,{},forged));
  if(mind)check(!actor.commit_energy_hex_zero(life,owners,{},event)); // Cannot omit an active co-observer.
  const auto result=actor.commit_energy_hex_zero(life,owners,mind_owner,event);
  check(result&&result->damage.size()==2&&result->ended.size()==(mind?3:2)&&result->damage[0].damage==63&&result->damage[1].damage==41);
  check(!actor.hex_effect(life,77)&&!actor.hex_effect(life,78)&&!actor.pending_energy_zero(life,99));
  check(actor.actor(life)->health->current==988); // Ending proposals do not apply shadow Health damage.
  for(const auto& hit:result->damage)check(actor.apply_damage(life,{Kind::armor_ignoring,double(hit.damage),20,hit.owner.skill},2500).reason==Reason::accepted);
  if(mind){check(result->mind_wrack&&result->mind_wrack->zero_damage==83);check(actor.apply_damage(life,{Kind::armor_ignoring,83,20,49},2500).reason==Reason::accepted);}
  check(actor.advance_conditions(life,10000).has_value()&&actor.actor(life)->health->current==(mind?801:884));
  check(!actor.commit_energy_hex_zero(life,owners,{},event));
  check(actor.gain_energy(life,1));const auto renewed=*skills::effects::energy_degeneration_hex(125,12);
  check(actor.apply_energy_degeneration_hex(life,source,renewed,79,10000).has_value());
  check(actor.advance_conditions(life,13000).has_value());
  const skills::effects::EnergyHexOwnership next{source,actor.hex_scope(),79,0,125,12,20,10000,10000+renewed.duration};
  check(!actor.commit_energy_hex_zero(life,{next},{},*actor.pending_energy_zero(life,99)));
  check(actor.commit_energy_hex_zero(life,{next},{},*actor.pending_energy_zero(life,100)).has_value());
 }
 for(bool die:{false,true}){
  LiveActor actor(profile(1),life);check(actor.apply_energy_degeneration_hex(life,source,*skills::effects::energy_degeneration_hex(125,12),77,1000).has_value());
  check(!actor.refresh_hexes(life,40000)&&actor.actor(life)->health->current==1000&&actor.hex_effect(life,77));
  check(!actor.clear_hexes(life,10000)&&actor.actor(life)->health->current==1000&&actor.hex_effect(life,77));
  check(actor.advance_conditions(life,4000).has_value());
  check(!actor.clear_hexes(life,4000)&&actor.pending_energy_zero(life,99));
  check(!actor.refresh_hexes(life,40000)&&actor.hex_effect(life,77)&&actor.pending_energy_zero(life,99));
  check(actor.apply_weapon_damage(life,2000)->killed);
  if(die)actor.refresh_hexes(life,10000);else check(actor.clear_hexes(life,10000));
  check(!actor.pending_energy_zero(life,99)&&!actor.hex_effect(life,77));
  check(actor.advance_conditions(life,11000).has_value()&&actor.actor(life)->health->current==0);
 }
 for(bool direct:{false,true}){
  LiveActor actor(profile(3),life);const auto plan=*skills::effects::energy_degeneration_hex(125,2);
  check(plan.duration==9000&&actor.apply_energy_degeneration_hex(life,source,plan,77,1000).has_value());
  if(!direct)check(actor.advance_conditions(life,10000).has_value()&&actor.pending_energy_zero(life,99));
  actor.refresh_hexes(life,10000);check(!actor.hex_effect(life,77)&&!actor.pending_energy_zero(life,99));
  check(actor.advance_conditions(life,12000).has_value()&&actor.actor(life)->health->current==964);
 }
 for(bool exact:{false,true}){
  LiveActor actor(profile(1),life);check(actor.apply_energy_degeneration_hex(life,source,*skills::effects::energy_degeneration_hex(125,12),77,1000).has_value());
  check(!actor.remove_hex_effect(life,77,exact?4000:10000)&&actor.hex_effect(life,77));
  if(exact)check(actor.pending_energy_zero(life,99)&&actor.actor(life)->health->current==976);
  else check(!actor.pending_energy_zero(life,99)&&actor.actor(life)->health->current==1000&&actor.energy(life)->current==1);
 }
 {
  LiveActor actor(profile(1),life);const auto plan=*skills::effects::energy_degeneration_hex(125,12);
  check(actor.apply_energy_degeneration_hex(life,source,plan,77,1000).has_value()&&actor.advance_conditions(life,2000));
  const auto before_units=*actor.precise_energy_units(life);const auto energy=*actor.energy(life);
  check(before_units==200000&&actor.drain_energy_units(life,1,2000)==200000);
  check(actor.retain_discrete_energy_zero(life,{energy.current,energy.maximum,0,unsigned(before_units%300000)},{0,energy.maximum,0,0},2000));
  const auto event=*actor.pending_energy_zero(life,77);check(event.before.fractional_units==200000);
  const skills::effects::EnergyHexOwnership owner{source,actor.hex_scope(),77,0,125,12,20,1000,1000+plan.duration};
  check(actor.commit_energy_hex_zero(life,{owner},{},event).has_value()&&actor.advance_conditions(life,4000));
 }
 std::cout<<"PASS NPC125/140 recipient/zero transaction,discrete fractions,lifetime gates and protected frontier;native admission pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

