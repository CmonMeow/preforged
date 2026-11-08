#include "gw_blood_is_power.h"
#include "gw_blood_ritual.h"
#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Blood is Power line "+std::to_string(at.line()));
}
Definition definition(){Definition d;d.binding.map=146;d.binding.actor=100;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
 d.relation=Relation::allied_to_players;d.spawn_energy=Energy{0,100};d.energy_pips=2;return d;}
int main()try{
 const Lifetime life{1,1,7,146,100};const skills::effects::Scope caster{1,1,2};const auto plan=*skills::effects::blood_is_power(12);
 {
  LiveActor live(definition(),life);check(live.advance_energy(life,1000));
  check(live.apply_energy_enchantment(life,caster,plan,1,1000).has_value());
  const auto ritual=live.apply_energy_enchantment(life,caster,*skills::effects::blood_ritual(0),2,1000);
  check(ritual&&ritual->removed.empty()&&live.hex_effect(life,1)&&live.hex_effect(life,2));
  // Base2 + Power5 + Ritual3 gives10pips for8s,then7pips for2s.
  check(live.advance_conditions(life,11000).has_value()&&live.precise_energy_units(life)==9400000);
  check(live.refresh_hexes(life,11000)&&!live.hex_effect(life,1)&&!live.hex_effect(life,2));
 }
 {
  LiveActor live(definition(),life);check(live.advance_energy(life,1000));
  check(live.apply_energy_enchantment(life,caster,plan,1,1000).has_value());
  check(!live.advance_energy(life,16000)&&live.precise_energy_units(life)==0);
  check(live.advance_conditions(life,16000).has_value());check(live.precise_energy_units(life)==8000000);
  check(live.refresh_hexes(life,16000)&&!live.hex_effect(life,1));
  check(live.advance_energy(life,16000));
 }
 {
  LiveActor live(definition(),life);check(live.advance_energy(life,1000));check(live.apply_energy_enchantment(life,caster,plan,1,1000).has_value());
  check(live.remove_reviewed_enchantment(life,1,5000));check(live.advance_conditions(life,16000).has_value()&&live.precise_energy_units(life)==5000000);
 }
 {
  LiveActor live(definition(),life);check(live.advance_energy(life,1000));check(live.apply_energy_enchantment(life,caster,plan,1,1000).has_value());
  const auto renewed=live.apply_energy_enchantment(life,caster,*skills::effects::blood_is_power(0),2,5000);
  check(renewed&&renewed->removed.size()==1&&!live.hex_effect(life,1));
  check(live.advance_conditions(life,16000).has_value()&&live.precise_energy_units(life)==8000000);
 }
 {
  auto d=definition();d.spawn_health=combat::Health{1,1000};LiveActor live(d,life);check(live.advance_energy(life,1000));
  check(live.apply_energy_enchantment(life,caster,plan,1,1000).has_value());check(live.apply_condition(life,combat::conditions::Kind::disease,1,1000,10000));
  const auto death=live.advance_conditions(life,2000);check(death&&death->killed&&death->death_time==1125&&live.precise_energy_units(life)==87500);
  check(live.advance_energy(life,2000)&&live.refresh_hexes(life,2000));check(live.precise_energy_units(life)==87500&&!live.hex_effect(life,1));
  check(live.advance_conditions(life,4000).has_value()&&live.advance_energy(life,4000)&&live.precise_energy_units(life)==87500);
 }
 for(unsigned mode=0;mode<6;++mode){
  auto d=definition();if(mode==0)d.relation=Relation::hostile_to_players;if(mode==1)d.relation=Relation::neutral;
  if(mode==2){d.spawn_energy.reset();d.energy_pips.reset();}if(mode==3)d.energy_pips.reset();if(mode==4){d.spawn_energy=Energy{0,100};d.energy_pips=0;}
  LiveActor live(d,life);auto handle=life;if(mode==5)++handle.actor_generation;
  const auto result=live.apply_energy_enchantment(handle,caster,plan,1,1000);check(result.has_value()==(mode==4));
  if(mode==4)check(live.advance_conditions(life,16000).has_value()&&live.precise_energy_units(life)==5000000);
 }
 std::cout<<"PASS allied NPC Blood is Power exact Energy expiry/removal/renewal,death crossing,clock barrier,unknown facts and lifetimes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
