#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered NPC events line "+std::to_string(at.line()));}
int main()try{
 const combat::npcs::Lifetime life{1,1,2,146,100};const Scope caster{1,9,1};Registry cr(caster);
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;d.conditions_allowed=true;
 for(bool pvp:{false,true}){combat::npcs::LiveActor npc(d,life);
  check(bool(npc.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12,pvp),1,100,100,FeveredDazedSpan::attempted_duration)));
  check(npc.take_fevered_observations(life).empty());
  check(npc.apply_condition(life,Kind::blind,9,200,5000));auto events=npc.take_fevered_observations(life);check(events.size()==1&&events[0].sequence==1&&events[0].reaction.pulses.size()==1&&events[0].alive&&!events[0].reaction.dazed_applied);
  auto speculative=npc;check(speculative.apply_condition(life,Kind::poison,9,300,5000));check(npc.take_fevered_observations(life).empty());
  auto stale=life;stale.actor_generation++;check(speculative.take_fevered_observations(stale).empty());
  events=speculative.take_fevered_observations(life);check(events.size()==1&&events[0].sequence==2&&events[0].reaction.pulses.size()==(pvp?1:2)&&events[0].reaction.dazed_applied==!pvp&&speculative.has_condition(life,Kind::dazed,300)==!pvp);
  check(speculative.take_fevered_observations(life).empty());
  check(speculative.apply_condition(life,Kind::poison,9,400,10000)&&speculative.take_fevered_observations(life).empty());
  check(speculative.remove_condition(life,Kind::blind,500)&&speculative.take_fevered_observations(life).empty());
  check(speculative.remove_hex_effect(life,1,600));check(speculative.apply_condition(life,Kind::weakness,9,700,1000)&&speculative.take_fevered_observations(life).empty());
 }
 // Batched transfer and Shockwave use the same observer, rather than only
 // the direct single-condition API. Deep Wound still updates real capacity.
 {combat::npcs::LiveActor target(d,life);check(bool(target.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12),1,100,100,FeveredDazedSpan::attempted_duration)));
  combat::conditions::State source;for(auto kind:{Kind::blind,Kind::poison,Kind::deep_wound})check(source.apply(kind,9,100,10000));
  check(bool(target.receive_conditions(life,*condition_transfer(154,12),source,9,200)));
  auto batch=target.take_fevered_observations(life);check(batch.size()==1&&batch[0].reaction.pulses.size()==4&&target.has_condition(life,Kind::dazed,200)&&target.actor(life)->health->maximum==400);
 }
 {auto earth=d;earth.earth_armor=60;combat::npcs::LiveActor target(earth,life);check(bool(target.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12),1,100,100,FeveredDazedSpan::attempted_duration)));
  check(bool(target.apply_earth_aoe(life,*caster_earth_aoe(937,0),0,20,9,200)));
  auto batch=target.take_fevered_observations(life);check(batch.size()==1&&batch[0].reaction.pulses.size()==4&&target.has_condition(life,Kind::dazed,200)&&target.actor(life)->health->maximum==500);
 }
 // Incoming poison survives its Fragility hit, but the separately generated
 // Dazed gain kills. Journal signals the dead target; no nested Dazed loop.
 d.spawn_health=combat::Health{30,500};combat::npcs::LiveActor npc(d,life);
 check(bool(npc.apply_fevered_dreams(life,cr,caster,*fevered_dreams(12),1,100,100,FeveredDazedSpan::attempted_duration)));
 check(npc.apply_condition(life,Kind::blind,9,100,10000));npc.take_fevered_observations(life);
 check(bool(npc.apply_fragility(life,*fragility(19,12),9,20,2,100)));
 check(npc.apply_condition(life,Kind::poison,9,200,10000));auto events=npc.take_fevered_observations(life);
 check(events.size()==1&&!events[0].alive&&events[0].reaction.dazed_applied&&npc.actor(life)->health->current==0&&npc.take_fragility_hits(life).size()==2);
 std::cout<<"PASS native NPC Fevered ongoing new-condition observation, one Dazed reaction, PvP exclusion, renewal/removal silence, staged journal isolation and lethal Fragility\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
