#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earth Shaker World line "+std::to_string(at.line()));
}
world::Instance::Npc body(unsigned id,float x,bool hostile=true,std::optional<bool> susceptible=true,unsigned health=1000){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
 d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::neutral;
 d.spawn_health=combat::Health{double(health),1000};d.damageable=true;d.conditions_allowed=true;d.physical_armor=60;
 d.knockdown_allowed=susceptible;d.knockdown_evidence=susceptible.has_value()?"fixture":"";n.combat_definition=d;return n;
}
void join(world::Instance& w,unsigned id,float x,unsigned team=2){
 w.join(id,"Adjacent",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,team);
 w.set_vitals(id,{1000,1000,30,30},100);
}
struct Fixture {
 world::Instance world;skills::effects::Scope source,target;combat::weapon::Owned weapon;
 Fixture(bool npc=false,unsigned health=1000,bool susceptible=true,bool unknown_secondary=false){
  std::vector<world::Instance::Npc> population{body(101,226,true,unknown_secondary?std::nullopt:std::optional(true)),
   body(102,100,true,false),body(103,60,false),body(104,-107)};
  if(npc)population.push_back(body(100,60,true,susceptible,health));
  world.configure_npcs(population);join(world,1,0,1);
  if(!npc){join(world,2,60);world.set_vitals(2,{health,1000,30,30},100);}
  source=*world.effect_scope(1);target=*world.effect_scope(npc?100:2);
  check(world.energy_capacity(1,1,0,100,{10,-2}));if(!npc)check(world.physical_armor(target,60));
  weapon={12,1,{20,20,2},12,false,false,15};check(world.ordinary_weapon(source,weapon,20));
 }
};
int main()try{
 using namespace combat::weapon;
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  //0 hit,1 low-rank failure,2 Blind miss,3 lethal primary,4 immune primary,
  //5 already-knocked-down primary,6 successful low-rank roll.
  Fixture f(npc,mode==3?1:1000,mode!=4);
  auto& w=f.world;
  join(w,3,226);join(w,4,-107);join(w,5,60,1);join(w,6,100);
  const auto edge=*w.effect_scope(3),balanced=*w.effect_scope(6);
  check(w.status_mutation(balanced,371,123,12,100,18100,true));
  if(mode==4&&!npc)check(w.status_mutation(f.target,371,124,12,100,18100,true));
  if(mode==5){
   if(npc)check(w.knockdown_npc(f.target,2000,100)==combat::npcs::control::Knockdown::accepted);
   else check(w.gale(f.source,f.target,12,{},100));
  }
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));
  unsigned failure_draws=0,miss_draws=0;
  w.configure_earth_shaker_failure_draws([&]{++failure_draws;return mode==1?.499:.5;});
  w.configure_attack_miss_draws([&]{++miss_draws;return 0.;});
  const auto plan=*earth_shaker_attack_plan(mode==1||mode==6?4:12);
  check(w.melee_attack(f.source,f.target,f.weapon,plan,1000));
  const bool hit=mode!=1&&mode!=2;
  check(w.knocked_down(edge,1000)==hit);
  check(w.knocked_down(*w.effect_scope(101),1000)==hit);
  for(unsigned id:{1u,4u,5u,6u,102u,103u,104u})check(!w.knocked_down(*w.effect_scope(id),1000));
  check(w.resources(3)->health==1000&&w.npc_combat_snapshot(*w.effect_scope(101))->health->current==1000);
  check(failure_draws==((mode==1||mode==6)?1u:0u));
  check(miss_draws==(mode==2?1u:0u));
  auto outcomes=w.take_attack_outcomes(f.source);
  check(outcomes.size()==1&&outcomes[0].target==f.target);
  check(outcomes[0].outcome==(mode==1?AttackOutcome::cancelled:mode==2?AttackOutcome::miss:AttackOutcome::hit));
  if(mode==0){check(w.knocked_down(edge,2999));check(!w.knocked_down(edge,3000));}
 }
 // Primary block suppresses secondary knockdowns without side attacks.
 {Fixture f;auto& w=f.world;join(w,3,100);
  check(w.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});
  check(w.melee_attack(f.source,f.target,f.weapon,*earth_shaker_attack_plan(12),1000));
  check(!w.knocked_down(f.target,1000)&&!w.knocked_down(*w.effect_scope(3),1000));
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==AttackOutcome::block);
 }
 // Failed skill is not a miss: Price of Failure must not damage the attacker.
 {Fixture f;auto& w=f.world;
  check(w.apply_price_of_failure(f.target,f.source,12,100));
  w.configure_earth_shaker_failure_draws([]{return 0.;});
  check(w.melee_attack(f.source,f.target,f.weapon,*earth_shaker_attack_plan(4),1000));
  check(w.resources(1)->health==1000&&w.resources(2)->health==1000);
 }
 // Unknown adjacent NPC susceptibility rejects before primary damage.
 {Fixture f(false,1000,true,true);auto& w=f.world;
  check(!w.melee_attack(f.source,f.target,f.weapon,*earth_shaker_attack_plan(12),1000));
  check(w.resources(2)->health==1000&&!w.knocked_down(f.target,1000));
 }
 std::cout<<"PASS Earth Shaker World primary-only weapon damage, target-centered controls, independent failure, misses/blocks, immunity and lethal primary\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

