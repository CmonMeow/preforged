#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <type_traits>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
static void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("NPC transition token line "+std::to_string(at.line()));}
static LiveActor body(bool with_energy=false,double health=40){Definition d;d.binding={146,100,100,0,0,0,0,20};d.relation=Relation::allied_to_players;
 d.spawn_health=Health{health,1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="authored token model";
 d.fleshy=true;d.fleshy_evidence="authored token model";d.physical_armor=60;
 if(with_energy){d.spawn_energy=Energy{0,100};d.energy_pips=4;}
 LiveActor a(d,{1,1,1,146,100});const auto l=a.lifetime();
 check(a.apply_condition(l,conditions::Kind::bleeding,1,100,1000));check(a.apply_condition(l,conditions::Kind::poison,1,100,1000));
 check(a.apply_healing_seed(l,{1,2,1},*skills::effects::healing_seed(12),80,100));
 check(bool(a.apply_opposed_owned_fragility(l,*skills::effects::fragility(19,12),1,20,81,100)));return a;}
int main()try{
 using Token=LiveActor::ConditionTransitionToken;static_assert(!std::is_default_constructible_v<Token>);static_assert(std::is_copy_constructible_v<Token>);
 for(auto horizon:{1100ull,10000ull}){
  auto a=body();const auto life=a.lifetime();const auto first=a.preview_condition_transition(life,horizon);check(bool(first));
  check(first->at()==1100&&first->kind()==Token::Kind::loss);check(first->before_transition().actor(life)->health->current==26);
  check(first->projected().actor(life)->health->current==9);check(a.actor(life)->health->current==40&&a.health_clock(life)==100);
  auto foreign=life;++foreign.actor_generation;check(!a.commit_condition_transition(foreign,*first));
  check(a.commit_condition_transition(life,*first)&&!a.commit_condition_transition(life,*first));
  check(a.take_fragility_hits(life).size()==1);check(a.gain_ally_health(life,skills::effects::healing_seed(12)->healing,1100));check(a.actor(life)->health->current==34);
  const auto second=a.preview_condition_transition(life,horizon);check(second&&second->at()==1100&&second->projected().actor(life)->health->current==17);
  check(a.commit_condition_transition(life,*second));check(a.take_fragility_hits(life).size()==1);
  check(a.gain_ally_health(life,skills::effects::healing_seed(12)->healing,1100));check(a.actor(life)->health->current==42);
  const auto finish=a.preview_condition_transition(life,horizon);check(finish&&finish->kind()==Token::Kind::finish);
  check(a.commit_condition_transition(life,*finish)&&!a.commit_condition_transition(life,*finish));check(!a.has_condition_frontier(life));
  check(bool(a.advance_conditions(life,horizon)));check(a.actor(life)->health->current==42&&a.take_fragility_hits(life).empty());
 }
 {auto a=body();const auto life=a.lifetime();auto token=a.preview_condition_transition(life,1100);check(bool(token));
  check(a.apply_condition(life,conditions::Kind::blind,1,1000,2000));const auto hp=a.actor(life)->health->current;
  check(!a.commit_condition_transition(life,*token)&&a.actor(life)->health->current==hp);}
 {auto a=body();const auto life=a.lifetime();auto token=a.preview_condition_transition(life,1100);check(bool(token));
  check(a.stage_condition_transition_boundary(life,*token)&&!a.stage_condition_transition_boundary(life,*token));
  check(a.actor(life)->health->current==26&&a.take_fragility_hits(life).empty());
  check(a.gain_ally_health(life,25,1100));const auto next=a.preview_condition_transition(life,1100);
  check(next&&next->projected().actor(life)->health->current==34);}
 {auto a=body(true);const auto life=a.lifetime();const auto token=a.preview_condition_transition(life,1100);check(bool(token));
  check(a.energy(life)->current==0);check(a.hold_condition_transition(life,*token)&&!a.hold_condition_transition(life,*token));
  const auto hp=a.actor(life)->health->current;check(!a.advance_conditions(life,1100)&&a.actor(life)->health->current==hp);
  check(!a.advance_energy(life,1100)&&a.energy(life)->current==0);
  check(!a.apply_condition(life,conditions::Kind::blind,1,1100,1000));
  check(!a.prepare_health_damage(life,{Kind::armor_ignoring,1,20,19},1100,true));
  const auto retry=a.preview_condition_transition(life,1100);check(bool(retry));check(a.commit_condition_transition(life,*retry));
  check(a.take_fragility_hits(life).size()==1&&a.actor(life)->health->current==9);
  check(a.energy(life)->current==1);check(a.advance_energy(life,1100)&&a.energy(life)->current==1);
 }
 {auto a=body(false,10);const auto life=a.lifetime();const auto token=a.preview_condition_transition(life,1100);check(bool(token));
  check(token->kind()==Token::Kind::regeneration_death&&token->integrated().death_time>100&&token->integrated().death_time<1100);
  check(!a.stage_condition_transition_boundary(life,*token)&&!a.hold_condition_transition(life,*token));
  check(a.actor(life)->health->current==10&&a.health_clock(life)==100);
  check(a.commit_condition_transition(life,*token)&&a.actor(life)->health->current==0&&!a.has_condition_frontier(life));
 }
 // False foreign adoption must not erase a valid lethal proposal's private
 // frontier provenance, needed by the ordinary current-registry rebase lane.
 {auto current=body();const auto life=current.lifetime();const auto token=current.preview_condition_transition(life,1100);check(bool(token));
  check(current.commit_condition_transition(life,*token));current.take_fragility_hits(life);
  auto proposed=current;const auto hit=proposed.apply_opposed_owned_damage(life,{Kind::armor_ignoring,100,20,19},1100);check(hit.reason==Reason::accepted&&hit.killed);
  Definition d;d.binding={146,101,101,0,0,0,0,20};d.relation=Relation::allied_to_players;d.spawn_health=Health{40,1000};d.damageable=true;
  LiveActor foreign(d,{1,1,2,146,101});check(!proposed.adopt_absorption_hit(current,foreign,life,1100));
  check(proposed.preserve_current_hexes(current,life,1100)&&proposed.actor(life)->health->current==0);
 }
 std::cout<<"PASS opaque full-lifetime/clock/effectcursor next-transition proposals; same-time interleaved gain, coarse equality and replay\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
