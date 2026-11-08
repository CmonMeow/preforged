// Separate model-only peer cases against pinned3658 or its explicit successor.
#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
static void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("frontier peer "+std::to_string(at.line()));}
static LiveActor body(double hp,bool hostile=false){
 Definition d;d.binding={146,100,100,0,0,0,0,20};d.relation=hostile?Relation::hostile_to_players:Relation::allied_to_players;
 d.spawn_health=Health{hp,1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="authored model";
 d.fleshy=true;d.fleshy_evidence="authored model";d.physical_armor=60;
 LiveActor a(d,{1,1,1,146,100});return a;
}
int main()try{
 using Token=LiveActor::ConditionTransitionToken;
 {auto a=body(20);const auto l=a.lifetime();
  check(a.apply_condition(l,conditions::Kind::bleeding,1,100,1000));
  check(a.apply_condition(l,conditions::Kind::poison,1,100,1000));
  check(a.apply_healing_seed(l,{1,2,1},*skills::effects::healing_seed(12),80,100));
  check(bool(a.apply_opposed_owned_fragility(l,*skills::effects::fragility(19,12),1,20,81,100)));
  const auto first=a.preview_condition_transition(l,10000);check(bool(first));
  check(first->before_transition().actor(l)->health->current==6);
  check(first->projected().actor(l)->health->current==0);
  check(a.commit_condition_transition(l,*first));const auto hits=a.take_fragility_hits(l);
  check(hits.size()==1&&hits[0].hit.applied==6&&hits[0].hit.damage==17);
  check(!a.has_condition_frontier(l)&&!a.preview_condition_transition(l,10000));
  check(!a.commit_condition_transition(l,*first));
 }
 {auto a=body(1000);const auto l=a.lifetime();
  check(a.apply_condition(l,conditions::Kind::bleeding,1,100,1000));
  check(a.apply_condition(l,conditions::Kind::poison,1,100,1000));
  check(bool(a.apply_opposed_owned_fragility(l,*skills::effects::fragility(19,12),1,20,81,100)));
  auto first=a.preview_condition_transition(l,1100);check(bool(first));check(a.commit_condition_transition(l,*first));
  check(a.take_fragility_hits(l).size()==1);const auto hp=a.actor(l)->health->current;
  check(!a.advance_conditions(l,1101));check(!a.gain_ally_health(l,25,1101));
  check(!a.apply_condition(l,conditions::Kind::blind,1,1101,1000));
  check(a.actor(l)->health->current==hp&&a.health_clock(l)==1100);
  check(a.gain_ally_health(l,25,1100));check(a.take_fragility_hits(l).empty());
  auto next=a.preview_condition_transition(l,1100);check(bool(next));check(a.commit_condition_transition(l,*next));
  check(a.take_fragility_hits(l).size()==1);
 }
 {auto a=body(1000,true);const auto l=a.lifetime();
  const auto phantom=*skills::effects::wound_spell(44,0);
  check(bool(a.apply_phantom_pain(l,phantom,1,80,100)));
  check(a.apply_condition(l,conditions::Kind::blind,1,100,phantom.hex_duration));
  check(bool(a.apply_fragility(l,*skills::effects::fragility(19,12),1,20,81,100)));
  const auto at=100+phantom.hex_duration;
  const auto first=a.preview_condition_transition(l,at+1000);check(first&&first->kind()==Token::Kind::loss);
  check(a.commit_condition_transition(l,*first));check(a.take_fragility_hits(l).size()==1);
  const auto tail=a.preview_condition_transition(l,at+1000);check(tail&&tail->kind()==Token::Kind::phantom_pain);
  check(a.commit_condition_transition(l,*tail));check(a.take_fragility_hits(l).size()==1);
  check(a.actor(l)->health->maximum==900);
  const auto finish=a.preview_condition_transition(l,at);check(finish&&finish->kind()==Token::Kind::finish);
  check(a.commit_condition_transition(l,*finish)&&!a.commit_condition_transition(l,*tail));
 }
 std::cout<<"PASS fatal transition, held future fence, same-time gain and Phantom Pain tail\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
