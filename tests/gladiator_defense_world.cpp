#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include "gw_npc_catalog.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Gladiator World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<7;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;check(w.ordinary_weapon(f.target,f.weapon,20));
  w.configure_attack_block_draws([&]{return mode==1?.9:.1;});
  check(w.status_mutation(f.target,372,123,12,100,10100,true));
  if(mode==2)check(w.status_mutation(f.target,137,124,0,100,8100,true));
  if(mode==3)check(w.status_mutation(f.target,372,123,12,200,{},false));
  if(mode==4)check(w.status_mutation(f.target,258,124,0,100,2100,true));
  if(mode==5)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==6)w.set_vitals(1,{20,1000,30,30},100);
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  const bool retaliation=mode==0||mode==6;
  if(w.resources(1)->health!=unsigned(mode==6?0:retaliation?971:1000))throw std::runtime_error("mode "+std::to_string(mode)+" health "+std::to_string(w.resources(1)->health));
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1);
  if(mode==0||mode==4||mode==6)check(outcomes.front().outcome==combat::weapon::AttackOutcome::block&&f.health()==1000);
  if(mode==5)check(outcomes.front().outcome==combat::weapon::AttackOutcome::miss&&f.health()==1000);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;check(w.ordinary_weapon(f.target,f.weapon,20));
  w.configure_attack_block_draws([]{return .1;});check(w.status_mutation(f.target,372,123,12,100,10100,true));
  check(w.ordinary_attack(f.source,2,200));w.tick(700);
  check(w.resources(1)->health==971&&f.health()==1000);
 }
 {warrior_test::Fixture f(330,false);auto& w=f.world;check(w.ordinary_weapon(f.target,f.weapon,20));
  w.configure_attack_block_draws([]{return .1;});check(w.status_mutation(f.target,372,123,12,100,10100,true));
  check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12),300));
  check(w.resources(1)->health==971&&f.health()==1000);
 }
 {warrior_test::Fixture f(399,false);auto& w=f.world;check(w.ordinary_weapon(f.target,{},20));
  w.configure_attack_block_draws([]{return .1;});check(w.status_mutation(f.target,372,123,12,100,10100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);
  check(w.resources(1)->health==1000&&f.health()==1000);
 }
 {warrior_test::Fixture f(330,false);auto& w=f.world;check(w.ordinary_weapon(f.target,{},20));
  w.join(3,"Unknown armor",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,30,30},100);
  const auto third=*w.effect_scope(3);w.configure_attack_block_draws([]{return .1;});
  check(w.status_mutation(f.target,372,123,12,100,10100,true));
  check(!w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12),300));
  check(w.resources(1)->health==1000&&f.health()==1000);
  check(w.physical_armor(third,60));check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12),400));
  check(w.resources(1)->health==971);
 }
 {const std::string hash(64,'a');
  std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/gladiator_defense_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);auto enemies=population.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{1000,1000,30,30},100);
  const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);check(w.physical_armor(owner,60)&&w.ordinary_weapon(owner,{},20));
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.status_mutation(owner,372,123,12,100,10100,true));w.configure_attack_block_draws([]{return .1;});
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
  w.damage_quest_player(attacker,owner,128,1000,strike,1000);
  if(w.resources(2)->health!=1000||w.npc_combat_snapshot(attacker)->health->current!=471)throw std::runtime_error("NPC retaliation: player "+std::to_string(w.resources(2)->health)+" npc "+std::to_string(w.npc_combat_snapshot(attacker)->health->current));
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={372,0};check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy=[&](auto amount){check(amount==5);return w.consume_energy(1,amount,f.source);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};
  h.status_mutation=[&](auto s,auto e,auto rank,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,rank,at,until,active);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[372]=1;attributes::Ranks ranks{};ranks[21]=12;
  check(runtime.start(1000,{372,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
  check(w.resources(1)->energy==25);auto effects=*w.status_snapshot(f.source,1000);check(effects.size()==1&&effects[0].expires==11000);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,631))==1);
  check(runtime.start(1001,{372,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Gladiator player Runtime,ordinary/melee/Cyclone retaliation,competing blocks,Rigor,miss,removal and lethal reaction\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
