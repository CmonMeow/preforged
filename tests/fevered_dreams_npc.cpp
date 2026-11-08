#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered NPC line "+std::to_string(at.line()));}
int main()try{
 const combat::npcs::Lifetime life{1,1,2,146,100};const Scope caster{1,9,1};Registry cr(caster);
 auto definition=[](std::uint32_t hp){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(hp),hp};d.damageable=true;d.conditions_allowed=true;return d;};
 for(bool pvp:{false,true}){combat::npcs::LiveActor npc(definition(500),life);
  check(npc.apply_condition(life,Kind::bleeding,9,100,10000));
  auto result=npc.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12,pvp),1,1,200,FeveredDazedSpan::attempted_duration);
  check(result&&result->alive&&result->reaction.dazed_applied==!pvp&&result->reaction.pulses.size()==(pvp?0:1));
  check((npc.activation_multiplier(life,28,200).value()==2.)==!pvp);
  auto forged=*fevered_dreams(12);forged.duration++;
  check(!npc.apply_fevered_dreams(life,cr,caster,forged,2,2,300,FeveredDazedSpan::attempted_duration));
  check(npc.apply_condition(life,Kind::blind,9,250,1000)); // failed future proposal did not advance clock
  auto stale=life;stale.actor_generation++;check(!npc.apply_fevered_dreams(stale,cr,caster,*fevered_dreams(12),2,2,300,FeveredDazedSpan::attempted_duration));
 }
 // Generated Dazed is a real condition gain: Fragility may kill the target.
 combat::npcs::LiveActor victim(definition(10),life);check(victim.apply_condition(life,Kind::blind,9,100,10000));
 check(bool(victim.apply_fragility(life,*fragility(19,12),9,20,2,100)));
 auto result=victim.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12),3,1,200,FeveredDazedSpan::attempted_duration);
 check(result&&!result->alive&&victim.actor(life)->health->current==0&&victim.take_fragility_hits(life).size()==1);
 std::cout<<"PASS native NPC Fevered application, PvP exclusion, Dazed casting effect, rollback and lethal Fragility settlement\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


