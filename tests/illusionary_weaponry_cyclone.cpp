#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("IW Cyclone line "+std::to_string(at.line()));
}
int main()try{
 for(bool far:{false,true})for(bool future:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u,102u}){
   world::Instance::Npc npc;npc.id=npc.definition.id=id;npc.x=far?-1000:id==100?-60:id==101?-120:-167;npc.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{480,480};d.damageable=true;npc.combat_definition=d;npcs.push_back(npc); // Armor deliberately unknown.
  }
  w.configure_npcs(npcs);
  for(unsigned id:{1u,2u,3u,4u,5u}){
   w.join(id,"Actor",id==1?0:far?1000:id==2?60:id==3?120:id==4?167:30,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
   w.set_vitals(id,{480,480,30,30},100);
  }
  const auto owner=*w.effect_scope(1);skills::Slots bar{};bar[0]={33,0};check(w.configure_adrenaline(owner,bar,100));check(w.status_mutation(owner,33,123,12,100,30100,true));
  check(w.apply_condition(owner,owner,combat::conditions::Kind::blind,100,10000));
  check(w.status_mutation(*w.effect_scope(2),453,124,12,100,10100,true));
  check(w.conjure_enchantment(owner,*skills::effects::conjure(182,12),100));
  combat::weapon::Owned axe{12,1,{20,20,2},12,false,false,2};check(w.ordinary_weapon(owner,axe,20));
  if(future)check(w.settle_resources(*w.effect_scope(3),200));
  unsigned draws=0;w.configure_attack_miss_draws([&]{++draws;return 0.;});w.configure_attack_block_draws([&]{++draws;return 0.;});
  const bool accepted=!future||far;
  check(w.melee_attack(owner,owner,axe,*combat::weapon::attack_skill(330,12),100)==accepted);
  const bool damaged=accepted&&!far;
  for(unsigned id:{2u,3u})check(w.resources(id)->health==(damaged?446u:480u));
  for(unsigned id:{4u,5u})check(w.resources(id)->health==480);
  for(unsigned id:{100u,101u})check(w.npc_combat_snapshot(*w.effect_scope(id))->health->current==(damaged?446:480));
  check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==480);
  const auto outcomes=w.take_attack_outcomes(owner);check(outcomes.size()==(damaged?4u:0u)&&draws==0);
  for(const auto& result:outcomes)check(result.outcome==combat::weapon::AttackOutcome::replaced&&!result.before_damage);
 }
 std::cout<<"PASS IW Cyclone mixed recipients,unknown armor,Blind/block bypass,range/teams,empty success and atomic future-clock rejection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
