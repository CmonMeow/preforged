#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered NPC lifecycle line "+std::to_string(at.line()));}
int main()try{
 const combat::npcs::Lifetime life{1,1,2,146,100};const Scope caster{1,9,1},replacement{1,10,2};Registry cr(caster),rr(replacement);
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;d.conditions_allowed=true;
 combat::npcs::LiveActor npc(d,life);check(npc.apply_condition(life,Kind::dazed,9,100,10000));
 check(bool(npc.apply_fragility(life,*fragility(19,12),9,20,7,100)));
 const auto hp=npc.actor(life)->health->current;
 for(auto span:{FeveredDazedSpan::attempted_duration,FeveredDazedSpan::retained_remaining}){
  auto copy=npc;auto result=copy.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12),1,1,200,span);
  check(result&&result->reaction.dazed_renewed&&result->reaction.pulses.size()==1);
  check(result->reaction.pulses[0].duration==(span==FeveredDazedSpan::attempted_duration?3000:9900));
  check(copy.actor(life)->health->current==hp&&copy.take_fragility_hits(life).empty());
  check(copy.has_condition(life,Kind::dazed,10099)&&!copy.has_condition(life,Kind::dazed,10100));
  auto renewed=copy.apply_fevered_dreams(life,rr,replacement,*fevered_dreams(0,true),2,2,300,span);
  check(renewed&&renewed->application.removed.size()==1&&renewed->application.removed[0].instance==1&&renewed->application.entry.cause==10&&renewed->application.entry.expires==5300&&!renewed->reaction.dazed_applied);
 }
 // Caster registry ownership is not an actor-number-only check.
 Registry stale({1,9,99});check(!npc.apply_fevered_dreams(life,stale,caster,*fevered_dreams(12),1,1,200,FeveredDazedSpan::attempted_duration));
 check(cr.apply(caster,{3,9,14,12,Category::stance,100,10000},Replace::category).accepted);
 auto extended=npc.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12),1,1,200,FeveredDazedSpan::attempted_duration);
 check(extended&&extended->application.entry.expires==29680&&extended->reaction.pulses[0].duration==3000);
 auto before=npc;check(!npc.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12),2,2,199,FeveredDazedSpan::attempted_duration));
 check(npc.actor(life)->health->current==before.actor(life)->health->current&&npc.hex_effect(life,1)->expires==before.hex_effect(life,1)->expires);
 std::cout<<"PASS NPC Fevered longer Dazed retention, no renewal Fragility, cross-caster alias replacement, Persistence and retired-registry rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
