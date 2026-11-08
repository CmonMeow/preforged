#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Uncredited NPC Energy hex death line "+std::to_string(at.line()));
}
int main()try{
 for(bool sparse:{false,true})for(bool reverse:{false,true})for(bool reuse:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{8,1000};
  d.spawn_energy=combat::npcs::Energy{20,20};d.energy_pips=0;d.damageable=d.conditions_allowed=true;
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,3u}){w.join(id,"Hex caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,100,100},100);}
  const auto killer=*w.effect_scope(reverse?3:1),linked=*w.effect_scope(reverse?1:3),target=*w.effect_scope(100);
  check(w.stage_energy_degeneration_hex(killer,target,*skills::effects::energy_degeneration_hex(125,12),20,1000));
  check(w.stage_energy_degeneration_hex(linked,target,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  w.tick(1500);w.leave(killer.actor);
  if(reuse){w.join(killer.actor,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},1500,killer.actor);
   w.set_vitals(killer.actor,{777,777,100,100},100);check(w.effect_scope(killer.actor)!=killer);}
  if(!sparse)for(unsigned at=1600;at<4000;at+=100)w.tick(at);
  w.tick(4000);check(w.npc_combat_snapshot(target)->health->current==0);
  check(w.resources(linked.actor)->health==998);
  if(reuse)check(w.resources(killer.actor)->health==777);
 }
 std::cout<<"8 uncredited NPC death cases: exact Malaise retirement, dense/sparse, actor order and ID reuse passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
