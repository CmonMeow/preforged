#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Action hex World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source;
 Fixture(unsigned hp=1000){std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?100:266;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(id==100?hp:1000),1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;population.push_back(n);}
  w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u,5u}){w.join(id,"Actor",id==1?0:id==2||id==5?100:id==3?266:267,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);w.set_vitals(id,{id==2?hp:1000u,1000,100,100},100);}
  source=*w.effect_scope(1);
 }
 effects::Scope scope(unsigned id){return *w.effect_scope(id);}
 double hp(unsigned id){return id<100?w.resources(id)->health:w.npc_combat_snapshot(scope(id))->health->current;}
};
int main()try{
 for(unsigned skill:{898u,899u})for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  Fixture f;const auto target=f.scope(npc?100:2);const auto plan=*effects::action_damage_hex(skill,12);
  if(mode){if(npc)check(bool(f.w.begin_npc_action(target,mode==1?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,100,mode==4?1000:2000,
   mode==1?std::optional<Slot>{}:std::optional<Slot>{Slot{mode==2?281u:320u,0}})));
   else if(mode==1)check(f.w.attack_activity(target,2000,100));
   else check(f.w.cast_activity(target,Slot{mode==2?281u:320u,0},100,mode==4?1000:2000));
  }
  const bool trigger=skill==898?mode==2||mode==3:mode==1||mode==3;
  std::vector<world::Packet> visuals;f.box->drain();
  check(f.w.stage_action_damage_hex(f.source,target,plan,20,1000,&visuals));
  const double initial=1000-(trigger?plan.damage:0);
  check(f.hp(target.actor)==initial);
  check(f.hp(npc?2:100)==(skill==898?initial:1000));
  check(f.hp(3)==(skill==898?initial:1000)&&f.hp(101)==(skill==898?initial:1000));
  check(f.hp(4)==1000&&f.hp(5)==1000&&f.hp(1)==1000);
  check(visuals.size()==1&&visuals[0]==*particles::targeted_body_2022(target.actor,1,skill));
  const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),visuals[0])==0);
  f.w.tick(2000);check(f.hp(target.actor)==initial-6);
  check(f.hp(npc?2:100)==(skill==898?initial:1000)); // Adjacent damage is not an area hex.
 }
 for(unsigned skill:{898u,899u}){
  Fixture f;const auto target=f.scope(2);check(f.w.cast_activity(target,281,100,2000));
  check(f.w.hex_breaker(target,*effects::hex_breaker(12),20,100));
  check(f.w.stage_action_damage_hex(f.source,target,*effects::action_damage_hex(skill,12),20,1000));
  check(f.hp(2)==1000&&f.hp(3)==1000&&f.hp(100)==1000&&f.hp(1)==961);
  check(f.w.status_snapshot(target,1000)->empty());
 }
 for(unsigned skill:{898u,899u}){
  Fixture f;const auto target=f.scope(2);const auto plan=*effects::action_damage_hex(skill,12);
  check(f.w.cast_activity(target,320,100,2000));
  check(f.w.protective_enchantment(target,target,*regeneration::reviewed(299,0),0,100));
  check(f.w.stage_action_damage_hex(f.source,target,plan,20,1000));
  check(f.hp(2)==1000-plan.damage+3);f.w.tick(2000);check(f.hp(2)==994-plan.damage+3);
 }
 for(unsigned skill:{898u,899u})for(bool npc:{false,true}){
  Fixture f;const auto target=f.scope(npc?100:2);const auto plan=*effects::action_damage_hex(skill,12);
  check(f.w.soul_barbs(f.source,target,*effects::soul_barbs(12),20,100));
  if(npc)check(bool(f.w.begin_npc_action(target,combat::npcs::control::Action::spell,200,2000,Slot{320,0})));
  else check(f.w.cast_activity(target,320,200,2000));
  check(f.w.stage_action_damage_hex(f.source,target,plan,20,1000));
  check(f.hp(target.actor)==1000-27-plan.damage);
  f.w.tick(2000);check(f.hp(target.actor)==994-27-plan.damage);
 }
 for(unsigned skill:{898u,899u}){
  Fixture f;const auto target=f.scope(2);auto bad=*effects::action_damage_hex(skill,12);++bad.damage;
  check(!f.w.stage_action_damage_hex(f.source,target,bad,20,1000));
  auto stale=target;++stale.generation;check(!f.w.stage_action_damage_hex(f.source,stale,*effects::action_damage_hex(skill,12),20,1000));
  check(f.hp(2)==1000&&f.hp(3)==1000&&f.w.status_snapshot(target,1000)->empty());
 }
 {Fixture f;const auto target=f.scope(100);check(bool(f.w.begin_npc_action(target,combat::npcs::control::Action::spell,100,2000)));
  check(!f.w.stage_action_damage_hex(f.source,target,*effects::action_damage_hex(898,12),20,1000));check(f.hp(100)==1000&&!(f.w.npc_combat_snapshot(target)->effects&0x800));
 }
 for(unsigned skill:{898u,899u}){
  Fixture f(10);const auto target=f.scope(100);check(bool(f.w.begin_npc_action(target,combat::npcs::control::Action::spell,100,2000,Slot{320,0})));
  check(f.w.stage_action_damage_hex(f.source,target,*effects::action_damage_hex(skill,12),20,1000));
  check(f.hp(100)==0&&f.hp(101)==(skill==898?937:1000));
  const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths[0].time==1000);
  f.w.tick(2000);check(f.hp(100)==0&&f.hp(101)==(skill==898?937:1000)&&f.w.take_npc_deaths(f.source).empty());
 }
 for(bool leaving:{false,true})for(bool explicit_tick:{false,true}){
  world::Instance w;world::Instance::Npc guard;guard.id=guard.definition.id=100;guard.x=leaving?1000:0;guard.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::neutral;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;guard.combat_definition=d;w.configure_npcs({guard});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto caster=*w.effect_scope(1),primary=*w.effect_scope(2),guardian=*w.effect_scope(100);
  auto enemy=guard;enemy.x=leaving?100:400;enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
  check(w.begin_wave_objective(caster,guardian,73,1,3,1)&&w.spawn_quest_wave(caster,73,1,{enemy},[]{return true;}));
  const auto moving=*w.effect_scope(60000);check(w.chase_quest_guardian(moving,100,100));
  check(w.cast_activity(primary,281,100,3000));if(explicit_tick)w.tick(2100);
  check(w.stage_action_damage_hex(caster,primary,*effects::action_damage_hex(898,12),20,2100));
  check(w.npc_combat_snapshot(moving)->health->current==(leaving?1000:937));
 }
 std::cout<<"PASS trusted Overload/Images World player/NPC action predicates,primary degeneration,conditional adjacency,boundary/friendly filters,HexBreaker and deferred target particles;Runtime/rendered verification separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

