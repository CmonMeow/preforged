#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Elemental attack skill line "+std::to_string(at.line()));}
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
   if(type!=5)check(!w.melee_target(owner,2,100,322));
   check(w.elemental_armor(target,60,60)&&w.lightning_armor(target,60)&&w.combat_armor(target,60));}
  const auto attack=*combat::weapon::attack_skill(type==5?394:322,0);
  box->drain();check(w.melee_target(owner,target.actor,100,attack.skill)&&w.melee_attack(owner,target,weapon,attack,100));
  if(type==5){check(npc_target?w.npc_combat_snapshot(target)->health->current==480:w.resources(2)->health==480);w.tick(200);}
  const auto health=npc_target?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);
  const double base=type==5?25:30;if(health!=480-base-(raw==11?0:17))std::cerr<<"type="<<type<<" raw="<<raw<<" npc="<<npc_target<<" health="<<health<<'\n';check(health==480-base-(raw==11?0:17));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==unsigned(raw!=11));
 }
 for(bool npc_target:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc_target){world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=10;npc.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
   d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{30,480};d.damageable=d.conditions_allowed=true;d.fire_armor=60;npc.combat_definition=d;w.configure_npcs({npc});}
  w.join(1,"Warrior",0,0,box,0,{}, {},100,1);w.set_vitals(1,{480,480,20,20},100);const auto owner=*w.effect_scope(1);
  if(!npc_target){w.join(2,"Foe",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{30,480,20,20},100);check(w.elemental_armor(*w.effect_scope(2),60,{}));}
  const auto target=*w.effect_scope(npc_target?100:2);check(w.conjure_enchantment(owner,*conjure(182,12),100));combat::weapon::Owned sword{12,1,{20,20,5},12,false,false,27};
  check(w.ordinary_weapon(owner,sword,20)&&w.melee_attack(owner,target,sword,*combat::weapon::attack_skill(382,12),100));
  check(npc_target?w.npc_combat_snapshot(target)->health->current==0:w.resources(2)->health==0);
  check(!w.has_condition(target,combat::conditions::Kind::bleeding,100));if(npc_target)check(w.take_npc_deaths(owner).size()==1);
 }
 std::cout<<"PASS trusted elemental Power Attack/Power Shot,correct armor/absorption/Conjure and lethal Sever Artery bonus without corpse condition;multi-target/equipment pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
