#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("NPC Parasitic Bond regression line "+std::to_string(at.line()));}
combat::npcs::Definition definition(){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};d.damageable=true;d.conditions_allowed=true;return d;}
int main()try{
 using namespace combat::npcs;
 for(unsigned rank:{0u,12u,20u})for(bool late:{false,true}){
  LiveActor n(definition(),{1,1,1,146,100});const auto life=n.lifetime();
  check(n.apply_parasitic_bond(life,rank,42,1,100).has_value());
  check(n.hex_effect(life,1)->cause==42&&n.hex_effect(life,1)->expires==20100);
  if(!late){auto step=n.advance_conditions(life,1100);check(step&&step->after.current==98&&step->cause==42);}
  auto end=n.advance_conditions(life,40100);check(end&&end->after.current==60&&!end->killed);
  n.refresh_hexes(life,40100);check(!n.hex_effect(life,1));
 }
 for(unsigned cadence:{1u,100u,40000u}){
  auto d=definition();d.spawn_health=combat::Health{10,100};LiveActor n(d,{1,1,1,146,100});const auto life=n.lifetime();
  check(n.apply_parasitic_bond(life,0,42,1,100).has_value());std::optional<std::uint64_t> death;
  for(std::uint64_t at=100+cadence;at<=40100;at+=cadence){const auto update=n.advance_conditions(life,at);if(update&&update->killed)death=update->death_time;}
  check(death==5100);
 }
 {
  auto d=definition();d.spawn_health=combat::Health{10,100};LiveActor n(d,{1,1,1,146,100});auto life=n.lifetime();
  check(n.apply_parasitic_bond(life,12,42,1,100).has_value());auto end=n.advance_conditions(life,20100);
  check(end&&end->killed&&end->death_time==5100&&end->cause==42&&end->after.current==0);
 }
 {
  LiveActor n(definition(),{1,1,1,146,100});auto life=n.lifetime();
  check(n.apply_parasitic_bond(life,0,42,1,100).has_value());
  const auto renewal=n.apply_parasitic_bond(life,12,42,2,1100);check(renewal&&renewal->removed.size()==1&&renewal->removed[0].instance==1);
  check(n.advance_conditions(life,21100)->after.current==58);
 }
 {
  LiveActor n(definition(),{1,1,1,146,100});auto life=n.lifetime();
  check(n.apply_parasitic_bond(life,0,42,1,100).has_value()&&n.remove_hex_effect(life,1,1100));
  check(n.advance_conditions(life,40100)->after.current==98);
  auto stale=life;++stale.actor_generation;check(!n.apply_parasitic_bond(stale,0,42,2,40100));
  check(!n.apply_parasitic_bond(life,21,42,2,40100)&&!n.apply_parasitic_bond(life,0,0,2,40100));
 }
 for(unsigned policy:{0u,1u,2u}){
  auto d=definition();if(policy==0){d.relation=Relation::protected_ally;d.damageable=false;d.conditions_allowed=false;}if(policy==1)d.damageable=false;if(policy==2){d.spawn_health.reset();d.damageable=false;d.conditions_allowed=false;}
  LiveActor n(d,{1,1,1,146,100});check(!n.apply_parasitic_bond(n.lifetime(),0,42,1,100));
 }
 std::cout<<"PASS NPC recipient Parasitic Bond degeneration,lifetime,renewal,removal and exact death time;world return integration pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
