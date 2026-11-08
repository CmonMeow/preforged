#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Elemental melee line "+std::to_string(at.line()));}
int main()try{
 for(unsigned type:{2u,5u,15u,27u})for(unsigned raw:{3u,4u,5u,11u})for(bool npc_target:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc_target){world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=10;npc.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{480,480};d.damageable=true;
   d.physical_armor=500;d.physical_armor_evidence="test";d.fire_armor=d.cold_armor=d.lightning_armor=d.earth_armor=60;npc.combat_definition=d;w.configure_npcs({npc});}
  w.join(1,"Warrior",0,0,box,0,{}, {},100,1);w.set_vitals(1,{480,480,20,20},100);
  const auto owner=*w.effect_scope(1);
  if(!npc_target){w.join(2,"Foe",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{480,480,20,20},100);}
  const auto target=*w.effect_scope(npc_target?100:2);
  const auto skill=raw==3?207u:raw==4?221u:182u;check(w.conjure_enchantment(owner,*conjure(skill,12),100));
  combat::weapon::Owned weapon{12,1,{20,20,raw},12,false,false,type};check(w.ordinary_weapon(owner,weapon,20)&&w.configure_projectiles({100,2}));
  if(!npc_target){check(w.physical_armor(target,500)&&w.physical_absorption(target,3));check(!w.ordinary_attack(owner,2,100));
   check(w.elemental_armor(target,60,60)&&w.lightning_armor(target,60)&&w.combat_armor(target,60));}
  box->drain();check(w.ordinary_attack(owner,target.actor,100));w.tick(600);if(type==5){check(npc_target?w.npc_combat_snapshot(target)->health->current==480:w.resources(2)->health==480);w.tick(700);}w.cancel_ordinary_attack(owner);
  const auto health=npc_target?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);
  check(health==(raw==11?460.:443.));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==unsigned(raw!=11));
 }
 std::cout<<"PASS trusted elemental sword/axe/hammer/bow ordinary hits use elemental armor,skip physical absorption and trigger matching Conjure for player/NPC;equipment/attack-skill routing pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
