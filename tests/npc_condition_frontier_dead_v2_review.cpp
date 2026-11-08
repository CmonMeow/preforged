// Additional successor-only negative test: dead integration is not a gain boundary.
#include "gw_npc_combat_catalog.h"
#include <iostream>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
int main(){
 Definition d;d.binding={146,100,100,0,0,0,0,20};d.relation=Relation::allied_to_players;
 d.spawn_health=Health{10,1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;
 d.healing_evidence="authored model";d.fleshy=true;d.fleshy_evidence="authored model";d.physical_armor=60;
 LiveActor a(d,{1,1,1,146,100});const auto l=a.lifetime();
 if(!a.apply_condition(l,conditions::Kind::bleeding,1,100,1000)||!a.apply_condition(l,conditions::Kind::poison,1,100,1000))return 1;
 const auto token=a.preview_condition_transition(l,1100);
 if(!token||token->kind()!=LiveActor::ConditionTransitionToken::Kind::regeneration_death)return 2;
 if(a.stage_condition_transition_boundary(l,*token))return 3;
 if(a.actor(l)->health->current!=10||a.has_condition_frontier(l))return 4;
 if(!a.commit_condition_transition(l,*token)||a.actor(l)->health->current!=0||a.has_condition_frontier(l))return 5;
 std::cout<<"PASS regeneration-death token cannot become incoming-gain held boundary\n";return 0;
}
