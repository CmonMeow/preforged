#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("NPC attack outcome regression line "+std::to_string(at.line()));}
int main()try{
 for(bool player_target:{false,true}){
  const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto pop=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/npc_attack_outcomes.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);
  auto enemies=pop.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.configure_attack_miss_draws([]{return .1;});
  w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{500,500,100,100},100);
  const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);check(w.physical_armor(owner,0));
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.apply_price_of_failure(owner,attacker,12,100));
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.base_roll=strike.miss_roll=.5;
  auto invalid=strike;++invalid.level;
  if(player_target)check(!w.damage_quest_player(attacker,owner,128,900,invalid,1000));
  else check(w.damage_quest_guardian(attacker,guard,{combat::Kind::armor_ignoring,10,8,0},128,900,invalid,1000).reason!=combat::Reason::accepted);
  check(w.take_attack_outcomes(attacker).empty()&&w.npc_combat_snapshot(attacker)->health->current==500);
  if(player_target)w.damage_quest_player(attacker,owner,128,1000,strike,1000);
  else w.damage_quest_guardian(attacker,guard,{combat::Kind::armor_ignoring,10,8,0},128,1000,strike,1000);
  const auto events=w.take_attack_outcomes(attacker);
  std::cout<<"player="<<player_target<<" NPCHealth="<<w.npc_combat_snapshot(attacker)->health->current<<" outcomes="<<events.size()<<'\n';
  check(w.npc_combat_snapshot(attacker)->health->current==463&&w.resources(2)->health==500&&w.npc_combat_snapshot(guard)->health->current==500);
  check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::miss&&events[0].attacker==attacker&&events[0].target==(player_target?owner:guard));
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
