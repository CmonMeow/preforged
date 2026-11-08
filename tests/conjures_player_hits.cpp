#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Conjure player hit line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{182u,207u,221u})for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Foe",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{id==2&&mode==4?10u:id==2&&mode==6?30u:480u,480,20,20},100);
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);const auto plan=*conjure(skill,12);
  check(w.conjure_enchantment(owner,plan,100));const auto effect=w.status_snapshot(owner,100)->front().instance;
  const auto raw=mode==3?(skill==182?3u:5u):skill==182?5u:skill==207?3u:4u;
  combat::weapon::Owned staff{12,1,{20,20,raw},12,false,true,26};
  check(w.ordinary_weapon(owner,staff,20)&&w.elemental_armor(target,60,60)&&w.lightning_armor(target,60)&&w.configure_projectiles({100,2}));
  const auto start=mode==2?59500u:100u,arrival=start+600;
  if(mode==5)check(w.apply_condition(target,owner,combat::conditions::Kind::blind,100,10000));
  if(mode==7){w.set_vitals(2,{480,480,0,40},100);check(w.energy_capacity(2,6,0,100,{20,-2}));check(w.status_mutation(target,skill==182?7:skill==207?8:9,1,0,100,30100,true));}
  const auto epoch=*w.control_epoch(target);
  box->drain();check(w.ordinary_attack(owner,2,start));
  if(mode==1)check(w.status_mutation(owner,skill,effect,12,200,{},false));
  w.tick(start+500);check(w.resources(2)->health==(mode==4?10u:mode==6?30u:480u));
  w.tick(arrival);w.cancel_ordinary_attack(owner);
  const bool bonus=mode==0||mode==6||mode==7;
  check(w.resources(2)->health==(mode==4||mode==6?0u:mode==5?480u:mode==7?453u:bonus?443u:460u));
  check(*w.control_epoch(target)-epoch==unsigned(mode==4||mode==6));
  if(mode==7)check(w.resources(2)->energy==4);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==unsigned(bonus));
  const auto before=w.resources(2)->health;w.tick(arrival+100);check(w.resources(2)->health==before);
 }
 for(unsigned skill:{182u,207u,221u})for(unsigned mode=0;mode<3;++mode){
  world::Instance w;world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=10;npc.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{mode==2?30.:480.,480};d.damageable=true;d.fire_armor=d.cold_armor=d.lightning_armor=60;
  npc.combat_definition=d;w.configure_npcs({npc});auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{480,480,20,20},100);
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);check(w.conjure_enchantment(owner,*conjure(skill,12),100));
  combat::weapon::Owned staff{12,1,{20,20,skill==182?5u:skill==207?3u:4u},12,false,true,26};check(w.ordinary_weapon(owner,staff,20)&&w.configure_projectiles({100,2}));
  box->drain();check(w.ordinary_attack(owner,100,100));
  if(mode==1)check(w.status_mutation(owner,skill,w.status_snapshot(owner,100)->front().instance,12,200,{},false));
  w.tick(600);check(w.npc_combat_snapshot(target)->health->current==(mode==2?30.:480.));w.tick(700);w.cancel_ordinary_attack(owner);
  check(w.npc_combat_snapshot(target)->health->current==(mode==2?0.:mode==1?460.:443.));
  check(w.take_npc_deaths(owner).size()==unsigned(mode==2));w.tick(800);check(w.take_npc_deaths(owner).empty());
 }
 std::cout<<"PASS three Conjures ordinary player/NPC projectile bonuses,expiry/dispel,weapon mismatch,miss,elemental Mantra,lethal base/bonus and single death credit;other attack paths pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
