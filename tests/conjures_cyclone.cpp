#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Conjure Cyclone line "+std::to_string(at.line()));}
int main()try{
 for(unsigned raw:{3u,4u,5u,11u})for(unsigned mode=0;mode<3;++mode){
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});auto box=std::make_shared<world::Outbox>();
  std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u,102u}){world::Instance::Npc npc;npc.id=npc.definition.id=id;npc.x=id==100?-60:id==101?-120:-167;npc.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{480,480};d.damageable=true;
   d.physical_armor=500;d.physical_armor_evidence="test";d.fire_armor=d.cold_armor=d.lightning_armor=d.earth_armor=60;
   if(id==101){d.fire_multiplier=2;d.fire_multiplier_evidence="test";}npc.combat_definition=d;npcs.push_back(npc);}
  w.configure_npcs(npcs);
  for(unsigned id:{1u,2u,3u,4u,5u}){w.join(id,"Actor",id==1?0:id==2?60:id==3?120:id==4?167:30,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);w.set_vitals(id,{480,480,20,20},100);}
  const auto owner=*w.effect_scope(1);const auto skill=raw==3?207u:raw==4?221u:182u;
  check(w.conjure_enchantment(owner,*conjure(skill,12),100));
  for(unsigned id:{2u,3u,4u,5u}){const auto target=*w.effect_scope(id);check(w.physical_armor(target,500)&&w.physical_absorption(target,3));
   if(mode!=2||id!=3)check(w.elemental_armor(target,60,60)&&w.lightning_armor(target,60)&&w.combat_armor(target,60));}
  combat::weapon::Owned axe{12,1,{20,20,raw},12,false,false,2};check(w.ordinary_weapon(owner,axe,20));
  if(mode==1)check(w.apply_condition(*w.effect_scope(2),owner,combat::conditions::Kind::blind,100,10000));
  box->drain();check(w.melee_attack(owner,owner,axe,*combat::weapon::attack_skill(330,12),100)==(mode!=2));
  const double damage=mode?0:raw==11?30:47;
  for(unsigned id:{2u,3u})check(w.resources(id)->health==480-damage);
  check(w.resources(4)->health==480&&w.resources(5)->health==480);
  check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==480-damage);
  check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==480-damage*(raw==5?2:1));
  check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==480);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==unsigned(!mode&&raw!=11?4:0));
  const auto outcomes=w.take_attack_outcomes(owner);check(outcomes.size()==(mode==2?0u:4u));
  for(const auto& outcome:outcomes)check(outcome.outcome==(mode==1?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
 }
 std::cout<<"PASS elemental Cyclone mixed recipients,one Conjure per hit,misses,range/team exclusion,atomic unknown armor and single fire-vulnerability application\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
