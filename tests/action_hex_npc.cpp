#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Action hex NPC line "+std::to_string(at.line()));
}
Definition profile(unsigned hp=1000){Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
 d.relation=Relation::hostile_to_players;d.spawn_health=Health{double(hp),1000};d.damageable=d.conditions_allowed=true;return d;}
int main()try{
 const Lifetime life{1,2,3,146,100};const skills::effects::Scope caster{1,1,1};
 for(unsigned skill:{898u,899u})for(unsigned action=0;action<5;++action)for(bool sparse:{false,true}){
  LiveActor live(profile(),life);const auto plan=*skills::effects::action_damage_hex(skill,12);
  if(action)check(bool(live.begin_action(life,action==1?control::Action::attack:control::Action::spell,100,2000,
   action==1?std::optional<skills::Slot>{}:std::optional<skills::Slot>{skills::Slot{action==2?281u:action==3?1u:320u,0}})));
  const bool trigger=skill==898?action>=2:action==1||action==4;
  const auto applied=live.apply_action_damage_hex(life,caster,plan,77,1000);check(bool(applied));
  check(applied->reaction.initial_damage==(trigger?plan.damage:0u));
  check(live.actor(life)->health->current==1000); // Damage intent is not independently executed.
  if(trigger){const auto hit=live.apply_damage(life,{Kind::armor_ignoring,double(plan.damage),20,skill},1000);check(hit.reason==Reason::accepted);}
  if(!sparse)for(unsigned at=1100;at<12000;at+=100)check(bool(live.advance_conditions(life,at)));
  const auto settled=live.advance_conditions(life,12000);check(bool(settled));
  const double expected=1000-(trigger?plan.damage:0)-2*plan.pips*(plan.duration/1000);
  check(std::abs(live.actor(life)->health->current-expected)<1e-7);
  check(live.refresh_hexes(life,12000)&&!live.hex_effect(life,77));
 }
 for(unsigned skill:{898u,899u}){
  LiveActor live(profile(),life);const auto plan=*skills::effects::action_damage_hex(skill,12);
  check(bool(live.apply_action_damage_hex(life,caster,plan,77,1000)));
  auto bad=plan;++bad.damage;check(!live.apply_action_damage_hex(life,caster,bad,78,2000));
  check(live.health_clock(life)==1000&&live.actor(life)->health->current==1000&&!live.hex_effect(life,78));
  check(live.remove_hex_effect(life,77,2000));check(std::abs(live.actor(life)->health->current-994)<1e-7);
  check(bool(live.advance_conditions(life,10000))&&std::abs(live.actor(life)->health->current-994)<1e-7);
  check(bool(live.apply_action_damage_hex(life,caster,plan,78,11000)));
 }
 {LiveActor live(profile(10),life);const auto plan=*skills::effects::action_damage_hex(898,12);
  check(bool(live.apply_action_damage_hex(life,caster,plan,77,1000)));
  const auto death=live.advance_conditions(life,5000);check(death&&death->killed&&death->death_time==2667&&death->cause==1);
 }
 {LiveActor live(profile(),life);check(bool(live.begin_action(life,control::Action::spell,100,2000)));
  check(!live.apply_action_damage_hex(life,caster,*skills::effects::action_damage_hex(898,12),77,1000));
  check(!live.health_clock(life)&&!live.hex_effect(life,77)); // Unknown skill action cannot partly advance target.
 }
 std::cout<<"PASS staged NPC Overload/Images authoritative action sampling,intents,dense/sparse degeneration,expiry,removal/reapply,death and atomic rejection;World transaction verification separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

