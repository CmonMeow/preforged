#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include "gw_npc_catalog.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Bonetti World line "+std::to_string(at.line()));}
void apply(warrior_test::Fixture& f,unsigned energy=20){Slots slots{};slots[0]={380,0};check(f.world.configure_adrenaline(f.target,slots,100));f.world.set_vitals(2,{1000,1000,energy,30},100);check(f.world.status_mutation(f.target,380,123,12,100,10100,true));}
int main()try{
 for(unsigned mode=0;mode<7;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;apply(f,mode==6?28:20);
  w.configure_attack_block_draws([&]{return mode==1?.9:.1;});
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==3)check(w.status_mutation(f.target,258,124,0,100,2100,true));
  if(mode==4)check(w.status_mutation(f.target,137,124,0,100,8100,true));
  if(mode==5)check(w.status_mutation(f.target,380,123,12,200,{},false));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  check(w.resources(2)->energy==unsigned(mode==0?25:mode==6?30:20));
 }
 {warrior_test::Fixture f(399,false);auto& w=f.world;apply(f);w.configure_attack_block_draws([]{return .1;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);check(w.resources(2)->energy==20&&f.health()==1000);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;apply(f);w.configure_attack_block_draws([]{return .1;});
  check(w.ordinary_attack(f.source,2,200));w.tick(700);check(w.resources(2)->energy==25);
 }
 for(unsigned mode=0;mode<3;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;apply(f);w.configure_attack_block_draws([]{return .1;});
  if(mode==0){check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),10100));check(w.resources(2)->energy==20);}
  if(mode==1){check(w.status_mutation(f.target,371,124,0,200,8200,true));check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));check(w.resources(2)->energy==20);}
  if(mode==2){check(w.kill(f.target,200));check(!w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));check(w.status_snapshot(f.target,300)->empty());}
 }
 {const std::string hash(64,'a');
  std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/bonetti_defense_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);auto enemies=population.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{1000,1000,20,30},100);
  const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);check(w.physical_armor(owner,60));Slots bar{};bar[0]={380,0};check(w.configure_adrenaline(owner,bar,100));
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.status_mutation(owner,380,123,12,100,10100,true));w.configure_attack_block_draws([]{return .1;});
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
  w.damage_quest_player(attacker,owner,128,1000,strike,1000);check(w.resources(2)->energy==25&&w.resources(2)->health==1000);
  w.damage_quest_player(attacker,owner,128,2000,strike,1000);check(w.resources(2)->energy==30&&w.npc_combat_snapshot(attacker)->health->current==500);
  const auto outcomes=w.take_attack_outcomes(attacker);check(outcomes.size()==2);for(const auto& e:outcomes)check(e.outcome==combat::weapon::AttackOutcome::block);
 }
 {warrior_test::Fixture f(330,false);auto& w=f.world;apply(f);w.configure_attack_block_draws([]{return .1;});
  w.join(3,"Unknown armor",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,30,30},100);
  check(!w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12),300));check(w.resources(2)->energy==20);
  check(w.physical_armor(*w.effect_scope(3),60));check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12),400));check(w.resources(2)->energy==25);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;apply(f,30);check(w.energy_capacity(2,2,0,100,{10,-2}));
  check(w.activate_overcast_energy(f.target,231,0,1,1000));w.configure_attack_block_draws([]{return .1;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),1501));
  const auto state=*w.overcast_energy_snapshot(f.target);check(overcast::precise_current(state)==20*overcast::energy_unit+49599);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;apply(f,0);check(w.energy_capacity(2,2,0,100,{10,-2}));check(w.energy_capacity(2,2,0,100,{0,-2}));
  check(w.activate_overcast_energy(f.target,231,0,1,1000));w.configure_attack_block_draws([]{return .1;});
  check(overcast::precise_current(*w.overcast_energy_snapshot(f.target))==-10*overcast::energy_unit);
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),1000));
  check(overcast::precise_current(*w.overcast_energy_snapshot(f.target))==-5*overcast::energy_unit);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={380,0};bar[1]={344,0};check(w.configure_adrenaline(f.source,bar,100));
  for(unsigned i=0;i<8;++i)check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200+i));
  check(w.adrenaline(f.source)->front().units==200);
  casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> packets;unsigned uses=0;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](auto n){check(n==0||n==5);return w.consume_energy(1,n,f.source);};
  h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};h.reconcile_adrenaline=[&](auto at){check(w.reconcile_adrenaline(f.source,at));};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto rank,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,rank,at,until,active);};
  h.begin_action=[&](Slot used,auto at){++uses;check(w.accepted_skill_start(f.source,used,at));};
  Policy p;p.primary=1;p.campaigns=255;p.learned[380]=p.learned[344]=1;attributes::Ranks ranks{};ranks[21]=12;
  check(runtime.start(1000,{380,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
  check(w.adrenaline(f.source)->front().units==0&&w.resources(1)->energy==30);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,639))==1);
  check(runtime.start(1001,{380,0,1},bar,p,ranks,true,h)==casting::Reason::adrenaline);
  check(w.status_snapshot(f.source,1001)->front().skill==380);
  check(runtime.start(2000,{344,0,1},bar,p,ranks,true,h)==casting::Reason::accepted&&uses==2);
  check(w.status_snapshot(f.source,2000)->front().skill==344);
 }
 std::cout<<"PASS Bonetti melee reward/cap,miss/other block/Rigor/removal/ranged exclusion,ordinary/Cyclone and Runtime200adrenaline/body639/accepted ending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
