#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Guardian NPC attack regression line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<3;++mode){
  const std::string hash(64,'a');
  std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/guardian_npc_attack.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);
  auto enemies=population.for_map(146);auto guardian=enemies.front();check(bool(guardian.combat_definition));
  guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});
  w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{100,500,100,100},100);
  const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);check(w.physical_armor(owner,60));
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.apply_insidious_parasite(owner,attacker,12,100));
  const auto protection=*skills::regeneration::reviewed(258,12);
  check(w.apply_health_regeneration(owner,2,protection,100,6100,1248,true,owner));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==0?0.:.9;});
  w.configure_attack_miss_draws([]{return .5;});
  if(mode==1)check(w.blindness(owner,attacker,*skills::effects::blindness(220,0),100));
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;
  strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
  const auto damage=combat::weapon::resolve(strike);check(bool(damage));
  w.damage_quest_player(attacker,owner,128,1000,strike,1000);
  check(draws==(mode==1?0u:1u));
  check(w.npc_combat_snapshot(attacker)->health->current==(mode==2?461:500));
  check(w.resources(2)->health==(mode==2?static_cast<unsigned>(std::ceil(139-damage->damage)):100u));
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1);
  using Outcome=combat::weapon::AttackOutcome;
  check(events[0].outcome==(mode==0?Outcome::block:mode==1?Outcome::miss:Outcome::hit));
  check(bool(events[0].before_damage)==(mode==2));
 }
 std::cout<<"PASS trusted NPC-to-player Guardian block/miss/hit distinction,draw consumption and Insidious pre-hit exclusion\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
