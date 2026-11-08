#include "gw_npc_failed_target.h"
#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
namespace failure=combat::ai::failed_target;
int main()try{
 const auto check=[](bool ok){if(!ok)throw std::runtime_error("NPC failed target decision");};
 combat::npcs::control::Token token{{1,100,1},0,1,combat::npcs::control::Action::spell,1000,2000,skills::Slot{191,17}};
 failure::Observation target{{1,2,1},skills::effects::Scope{1,2,1},true,true,true,true,0};
 check(failure::at_completion(token,target,1999).status==failure::Status::deferred);
 check(failure::at_completion(token,target,2000).status==failure::Status::valid);
 target.alive=false;target.clock=2001;
 check(failure::at_completion(token,target,2000).status==failure::Status::deferred);
 auto d=failure::at_completion(token,target,2001);
 check(d.status==failure::Status::failed&&d.reason==failure::Reason::dead);
 target.alive.reset();check(failure::at_completion(token,target,2001).status==failure::Status::deferred);
 target.alive=true;target.same_plane=false;
 check(failure::at_completion(token,target,2001).reason==failure::Reason::plane);
 target.same_plane=true;target.line_of_sight=false;
 check(failure::at_completion(token,target,2001).reason==failure::Reason::line_of_sight);
 target.current=skills::effects::Scope{1,2,2};
 check(failure::at_completion(token,target,2001).reason==failure::Reason::replaced);
 target.current.reset();check(failure::at_completion(token,target,2001).reason==failure::Reason::missing);
 token.skill=skills::Slot{186,0};check(failure::at_completion(token,target,2001).status==failure::Status::unsupported);
 token.skill=skills::Slot{194,0};check(failure::at_completion(token,target,2001).status==failure::Status::failed);
 target.clock=3000;check(failure::at_completion(token,target,2001).status==failure::Status::deferred);
 for(const unsigned skill:{31u,191u,194u,214u}){
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
  d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;
  combat::npcs::LiveActor npc(d,{1,1,1,146,100});const auto life=npc.lifetime();const skills::Slot slot{static_cast<std::uint16_t>(skill),17};
  check(npc.configure_spellbook(life,{{slot,1248,"authored failure regression"}}));
  const combat::ai::SpellTarget facts{{1,2,1},100,true,true,true,true};
  const auto first=*npc.begin_spell(life,slot,facts,1000);const auto paid=npc.energy(life)->current;
  check(!npc.fail_target_action(life,first,first.completes-1));
  auto wrong=first;++wrong.sequence;check(!npc.fail_target_action(life,wrong,first.completes));
  auto stale=life;++stale.actor_generation;check(!npc.fail_target_action(stale,first,first.completes));
  check(npc.advance_energy(life,first.completes+1));check(!npc.fail_target_action(life,first,first.completes));
  const auto done=first.completes+1;check(npc.fail_target_action(life,first,done));
  check(npc.energy(life)->current==paid&&!npc.fail_target_action(life,first,done));
  const combat::ai::Spellbook::Availability disabled=[](skills::Slot,std::uint64_t){return std::optional<std::uint64_t>{UINT64_MAX};};
  check(!npc.choose_spell(life,facts,done,disabled));check(npc.choose_spell(life,facts,done)==slot);
  const auto next=*npc.begin_spell(life,slot,facts,done);
  check(!npc.fail_target_action(life,first,next.completes));
  check(npc.active_action(life,done)==next);check(npc.complete_action(life,next,next.completes));
  check(!npc.choose_spell(life,facts,next.completes)); // successful aftercast remains authoritative
  const auto later=next.completes+6000;const auto interrupted=*npc.begin_spell(life,slot,facts,later);
  check(npc.cancel_action(life,later+1));check(!npc.fail_target_action(life,interrupted,interrupted.completes));
  if(skill==191)check(!npc.choose_spell(life,facts,interrupted.completes)); // interruption retains normal recharge
 }
 std::cout<<"PASS bounded NPC failed-target completion decisions\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

