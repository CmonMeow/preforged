#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tainted Flesh NPC line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<9;++mode){
  const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto pop=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/tainted_flesh_npc.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);
  auto enemies=pop.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  auto& d=*enemies.front().combat_definition;d.spawn_health=combat::Health{1,500};
  d.fleshy=mode==4?std::nullopt:std::optional(mode!=5);if(d.fleshy)d.fleshy_evidence="fixture";
  d.disease_immune=mode==6;d.disease_immunity_evidence="fixture";
  world::Instance w;w.configure_npcs({guardian});w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .1;});
  for(unsigned id=1;id<=2;++id){w.join(id,"Player",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{1000,1000,100,100},100);check(w.physical_armor(*w.effect_scope(id),60));}
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2),guard=*w.effect_scope(100);
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.tainted_flesh(caster,owner,*tainted_flesh(0),100));
  if(mode==1)check(w.blindness(caster,attacker,*blindness(220,0),100));
  if(mode==2)check(w.apply_health_regeneration(owner,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,owner));
  if(mode==7){w.leave(1);w.join(1,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{1000,1000,100,100},100);}
  if(mode==8)check(w.degeneration_hex(caster,attacker,*degeneration_hex(19,12),100));
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.base_roll=strike.miss_roll=.5;
  using combat::weapon::AttackDelivery;
  const auto delivery=mode==3?AttackDelivery::unknown:AttackDelivery::melee;
  check(!w.damage_quest_player(attacker,owner,128,900,strike,1000,static_cast<AttackDelivery>(99)));
  check(!w.damage_quest_player(attacker,owner,128,900,strike,1000,AttackDelivery::ranged));
  w.damage_quest_player(attacker,owner,128,1000,strike,1000,delivery);
  const bool infected=mode==0||mode==7||mode==8;
  check(w.has_condition(attacker,Kind::disease,1000)==infected);
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1&&events.front().delivery==delivery);
  if(infected){
   if(mode==8)check(w.npc_combat_snapshot(attacker)->health->current==0&&w.resources(2)->health<1000);
   else w.tick(1200);
   check(w.npc_combat_snapshot(attacker)->health->current==0);
   check(w.take_npc_deaths(caster).size()==unsigned(mode!=7)&&w.take_npc_deaths(owner).empty());
   if(mode==7)check(w.take_npc_deaths(*w.effect_scope(1)).empty());
  }else check(w.npc_combat_snapshot(attacker)->health->current==1);
 }
 std::cout<<"PASS authored NPC melee Tainted Flesh hit/miss/block,unknown delivery/flesh,nonflesh/immunity,Fragility death and original/reused-caster credit\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
