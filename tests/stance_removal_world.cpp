#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Stance removal line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={345,0};bar[1]={340,0};check(w.configure_adrenaline(f.target,bar,100));
  check(w.status_mutation(f.target,345,123,15,100,5100,true));
  if(mode==1)check(w.status_mutation(f.target,371,124,0,200,8200,true));
  auto target=f.target;if(mode==2)++target.generation;
  check(w.end_stance(target,123,mode==3?99:mode==4?5100:300)==(mode==0));
  if(mode==0){check(w.status_snapshot(f.target,300)->empty());for(const auto& p:*w.adrenaline(f.target))check(p.units==25);check(!w.end_stance(target,123,301));for(const auto& p:*w.adrenaline(f.target))check(p.units==25);}
  if(mode==1)check(w.status_snapshot(f.target,300)->front().skill==371);
  if(mode==2||mode==3)check(w.status_snapshot(f.target,300)->front().skill==345);
 }
 {warrior_test::Fixture f(331,false);auto& w=f.world;check(w.status_mutation(f.source,375,123,0,100,5100,true));
  check(w.weapon_attack_timing(f.source,200)->period_ms==1173);check(w.end_stance(f.source,123,300));check(w.weapon_attack_timing(f.source,300)->period_ms==1750);
 }
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
 d.spawn_health=combat::Health{500,500};d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;d.conditions_allowed=true;d.damageable=true;
 const combat::npcs::Lifetime life{1,2,3,146,100};
 for(unsigned skill:{16u,17u,455u}){combat::npcs::LiveActor npc(d,life);
  check(skill==455?npc.activate_storm_chaser(life,0,123,100):npc.apply_interrupt_stance(life,skill,0,123,100,1000));
  const auto energy=npc.energy(life);check(!npc.remove_stance(life,124,200));check(!npc.remove_stance(life,123,99));
  auto stale=life;++stale.actor_generation;check(!npc.remove_stance(stale,123,200));
  check(npc.remove_stance(life,123,200));check(!npc.newest_stance(life,200)&&npc.movement_multiplier(life,200)==1&&npc.energy(life)==energy);
  check(!npc.remove_stance(life,123,201));
 }
 std::cout<<"PASS exact stance end,Defensive removal reward once,replacement/stale/time rejection,Dwarven haste removal and NPC Mantra/Storm cleanup;Wild Blow contact binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
