#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Elemental NPC current authority line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{183u,189u,191u}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.spawn_health={1000,1000};d.damageable=true;d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  d.fire_armor=60;d.fire_armor_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Doomed caster",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1,1000,100,100},100);
  const auto source=*w.effect_scope(1),foe=*w.effect_scope(2),npc=*w.effect_scope(100);
  check(w.elemental_armor(foe,60,60));check(w.apply_parasitic_bond(foe,npc,12,100));
  check(w.npc_combat_snapshot(npc)->effects&0x800);
  const auto plan=*skills::effects::elemental(skill,0);
  check(w.elemental(source,plan.caster_center?source:foe,plan,20,100));check(w.resources(2)->health==0);
  const auto state=*w.npc_combat_snapshot(npc);
  if(skill==191){check(!(state.effects&0x800));check(state.health->current==1000);} // Excluded NPC still receives caster-death cleanup.
  else{check(!(state.effects&0x800));check(state.health->current==1000-plan.damage);}
 }
 std::cout<<"PASS elemental NPC current-state commit preserves death cleanup;single-target exclusion control\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
