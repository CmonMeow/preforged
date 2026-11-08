#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Insidious NPC regression line "+std::to_string(at.line()));}
int main()try{
 for(bool player_target:{false,true})for(bool missed:{false,true}){
  const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto pop=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/insidious_parasite_npc.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);
  auto enemies=pop.for_map(146);auto guardian=enemies.front();check(guardian.combat_definition.has_value());guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.configure_attack_miss_draws([]{return .5;});
  const auto box=std::make_shared<world::Outbox>();w.join(2,"Owner",100,0,box);w.set_vitals(2,{100,500,100,100},100);
  const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);check(w.physical_armor(owner,60));
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  box->drain();check(w.apply_insidious_parasite(owner,attacker,12,100));const auto application=box->drain();
  check(std::find(application.begin(),application.end(),*skills::particles::targeted_body_2022(attacker.actor,owner.actor,123))!=application.end());
  if(missed)check(w.blindness(owner,attacker,*skills::effects::blindness(220,0),100));
  check(guardian.combat_definition->physical_armor.has_value());
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.location_armor=player_target?60:*guardian.combat_definition->physical_armor;strike.base_roll=strike.miss_roll=.5;
  const auto damage=combat::weapon::resolve(strike);check(bool(damage));
  if(player_target)w.damage_quest_player(attacker,owner,128,1000,strike,1000);
  else w.damage_quest_guardian(attacker,guard,{combat::Kind::armor_ignoring,10,strike.level,0},128,1000,strike,1000);
  check(w.npc_combat_snapshot(attacker)->health->current==(missed?500:461));
  check(w.resources(2)->health==(missed?100u:static_cast<unsigned>(std::ceil(139-(player_target?damage->damage:0)))));
  check(w.npc_combat_snapshot(guard)->health->current==(missed||player_target?500:500-damage->damage));
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1&&events[0].outcome==(missed?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
  check(events[0].before_damage==(missed?std::nullopt:std::optional<std::uint64_t>{1000}));
  if(!player_target&&!missed){
   check(w.npc_combat_snapshot(attacker)->effects&0x800);
   check(w.kill(owner,1100));check(!(w.npc_combat_snapshot(attacker)->effects&0x800));
   const auto second=w.damage_quest_guardian(attacker,guard,{combat::Kind::armor_ignoring,10,strike.level,0},128,1200,strike,1000);check(second.reason==combat::Reason::accepted);
   check(w.npc_combat_snapshot(attacker)->health->current==461&&w.resources(2)->health==0);
  }
 }
 std::cout<<"PASS staged NPC Insidious player/guardian hit-before-damage transfer,miss exclusion,target230 publication and caster-death cleanup\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
