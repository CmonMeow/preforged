#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC world Parasitic Bond regression line "+std::to_string(at.line()));
}
std::unique_ptr<world::Instance> fixture(unsigned health=100){
 const std::string hash(64,'a');
 std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
 auto population=world::load_npc_catalog(appearance);
 std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+
  " tests/parasitic_bond_npc_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 "+std::to_string(health)+" 60 1 1 test test test - test 60 60 test test\n");
 const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);
 auto w=std::make_unique<world::Instance>();w->configure_npcs(population.for_map(146));
 w->join(1,"Necromancer",0,0,std::make_shared<world::Outbox>());w->set_vitals(1,{50,200,20,20},100);return w;
}
int main()try{
 for(unsigned rank:{0u,12u,20u})for(unsigned cadence:{100u,40000u}){
  auto w=fixture();const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
  check(w->apply_parasitic_bond(source,target,rank,100));
  for(std::uint64_t now=100+cadence;now<=40100;now+=cadence)w->tick(now);
  if(w->resources(1)->health!=80+6*rank||std::abs(w->npc_combat_snapshot(target)->health->current-60)>.000001)
   throw std::runtime_error("expiry rank="+std::to_string(rank)+" cadence="+std::to_string(cadence)+" source="+std::to_string(w->resources(1)->health)+" target="+std::to_string(w->npc_combat_snapshot(target)->health->current));
  w->tick(60100);check(w->resources(1)->health==80+6*rank);
 }
 for(unsigned cadence:{1u,100u,40000u}){
  auto w=fixture(10);const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
  check(w->apply_parasitic_bond(source,target,0,100));
  for(std::uint64_t now=100+cadence;now<=40100;now+=cadence)w->tick(now);
  const auto deaths=w->take_npc_deaths(source);
  check(deaths.size()==1&&deaths[0].time==5100&&w->resources(1)->health==80);
 }
 {
  auto w=fixture();const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
  check(w->apply_parasitic_bond(source,target,0,100));
  check(w->apply_parasitic_bond(source,target,12,1100));
  w->tick(20100);check(w->resources(1)->health==50);
  w->tick(21100);check(w->resources(1)->health==152&&w->npc_combat_snapshot(target)->health->current==58);
 }
 for(bool leave:{false,true}){
  auto w=fixture();const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
  check(w->apply_parasitic_bond(source,target,0,100));
  if(leave)w->leave(1);else check(w->kill(source,1100));
  w->tick(40100);check(w->npc_combat_snapshot(target)->health->current==(leave?100:98));
 }
 for(unsigned cadence:{100u,40000u}){
  auto w=fixture(10);const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
  check(w->apply_parasitic_bond(source,target,0,100));
  check(w->fire_storm(source,target,*skills::effects::fire_storm(0),20,100));
  for(std::uint64_t now=100+cadence;now<=40100;now+=cadence)w->tick(now);
  const auto deaths=w->take_npc_deaths(source);
  check(deaths.size()==1&&deaths[0].time==2100&&w->resources(1)->health==80);
 }
 {
  auto w=fixture();const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
  auto stale=target;++stale.generation;check(!w->apply_parasitic_bond(source,stale,0,100));
  check(w->apply_parasitic_bond(source,target,0,100));
  w->join(2,"Other caster",0,0,std::make_shared<world::Outbox>());w->set_vitals(2,{50,200,20,20},100);
  // Cross-caster behavior is unresolved;rejection must leave the existing
  // hex and return intact,never replace it without the documented extra heal.
  check(!w->apply_parasitic_bond(*w->effect_scope(2),target,12,1100));
  check(w->damage_npc(source,target,{combat::Kind::armor_ignoring,100,20,27},1248,2100).killed);
  check(w->resources(1)->health==80&&w->resources(2)->health==50);
  w->tick(40100);check(w->resources(1)->health==80&&w->take_npc_deaths(source).size()==1);
 }
 for(unsigned cadence:{100u,40000u}){
  auto w=fixture(10);w->set_vitals(1,{100,1000,20,20},100);
  w->join(2,"Condition source",0,0,std::make_shared<world::Outbox>());w->set_vitals(2,{100,1000,20,20},100);
  const auto source=*w->effect_scope(1),target=*w->effect_scope(100),other=*w->effect_scope(2);
  check(w->apply_condition(other,source,combat::conditions::Kind::bleeding,100,40000));
  check(w->apply_parasitic_bond(source,target,0,100));const auto before=*w->control_epoch(source);
  for(std::uint64_t now=100+cadence;now<=40100;now+=cadence)w->tick(now);
  const auto deaths=w->take_npc_deaths(source);
  check(w->resources(1)->health==0&&w->control_event_time(source,before)==21767&&deaths.size()==1&&deaths[0].time==5100);
 }
 std::cout<<"PASS NPC world Parasitic Bond expiry,death,renewal,caster lifecycle and mixed Health ordering\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
