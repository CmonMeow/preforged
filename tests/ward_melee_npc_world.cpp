#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Ward176 NPC World line "+std::to_string(at.line()));}
world::Instance::Npc make_ward_npc(unsigned id,bool spirit=false){
 world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.x=id==100?100:200;
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
 d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
 d.creature_type=spirit?"spirit":"human";d.creature_type_evidence="authored fixture";d.physical_armor=60;n.combat_definition=d;return n;
}
int main()try{
 using Outcome=combat::weapon::AttackOutcome;using Delivery=combat::ai::ordinary::Delivery;
 for(unsigned mode=0;mode<7;++mode){world::Instance w;w.configure_npcs({make_ward_npc(100,mode==3),make_ward_npc(101)});
  w.join(1,"Caster",mode==4?500:0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(100),enemy=*w.effect_scope(101);
  check(w.apply_ward_melee(owner,*ward_melee_plan(0),100));
  check(!w.ward_melee_source(enemy,100));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==1?.9:.1;});
  const combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,mode==2?Delivery::ranged:Delivery::melee,750,2000,128,{1200,32},"authored NPC block fixture"};
  const auto begins=mode==5?4350u:mode==6?4349u:1000u;
  check(w.configure_npc_opposed_ordinary(enemy,weapon,begins));check(bool(w.begin_npc_opposed_ordinary(enemy,ally,begins)));
  w.tick(begins+750);if(mode==2)w.tick(2100); // Release first, then resolve the ranged flight.
  const auto events=w.take_attack_outcomes(enemy);if(events.size()!=1)
   throw std::runtime_error("Ward176 NPC outcome count mode "+std::to_string(mode)+" count "+std::to_string(events.size()));
  const bool blocked=mode==0||mode==6;check(events[0].outcome==(blocked?Outcome::block:Outcome::hit));
  check(draws==unsigned(mode==0||mode==1||mode==6));check((w.npc_combat_snapshot(ally)->health->current==1000)==blocked);
 }
 // Mixed player sides need an exact NPC quest owner; no blanket NPC allies.
 {world::Instance w;auto ally_body=make_ward_npc(100);w.configure_npcs({ally_body});
  for(unsigned id:{1u,2u}){w.join(id,"Opposed",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),other=*w.effect_scope(2),ally=*w.effect_scope(100);
  check(w.apply_ward_melee(owner,*ward_melee_plan(0),100)&&w.apply_ward_melee(other,*ward_melee_plan(20),100));check(!w.ward_melee_source(ally,100));
  check(w.spawn_kill_encounter(owner,75,1,{make_ward_npc(101)},[]{return true;}));const std::array allies{ally};check(w.bind_npc_opposed_allies({owner,75,1},allies));
  check(w.ward_melee_source(ally,100)->owner==owner);check(!w.ward_melee_source(ally,5100));
 }
 // Actual NPC->player ordinary consumer, using an authored private quest AI.
 for(bool warded:{false,true}){world::Instance w;w.join(1,"Owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1);check(w.physical_armor(owner,0));auto body=make_ward_npc(101);body.x=0;body.combat_definition->binding.map=160;body.combat_definition->spawn_health=combat::Health{80,80};
  check(w.spawn_kill_encounter(owner,75,1,{body},[]{return true;}));const auto enemy=*w.effect_scope(60000);
  if(warded)check(w.apply_ward_melee(owner,*ward_melee_plan(0),100));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});check(w.arm_grawl_melee(owner,1,1000));
  check(w.damage_npc(owner,enemy,{combat::Kind::armor_ignoring,1,20,194},1248,1100).reason==combat::Reason::accepted);
  w.tick(1100);w.tick(2000);const auto events=w.take_attack_outcomes(enemy);check(events.size()==1);
  check(events[0].outcome==(warded?Outcome::block:Outcome::hit));check(draws==unsigned(warded));check((w.resources(1)->health==1000)==warded);
 }
 std::cout<<"Ward176 actual NPC->NPC/player melee block, ranged/spirit/area exclusions, exact expiry and owned team admission passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
