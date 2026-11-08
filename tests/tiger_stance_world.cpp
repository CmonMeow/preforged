#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Tiger Stance line "+std::to_string(at.line()));
}
int main()try{
 check(tiger_stance(0)&&tiger_stance(0)->duration==4000);
 check(tiger_stance(15)&&tiger_stance(15)->duration==10000);
 check(!tiger_stance(21));
 for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;
  check(!w.status_mutation(f.source,995,123,0,100,4099,true));
  check(w.status_mutation(f.source,995,123,0,100,4100,true));
  check(w.attack_duration(f.source,1000,200)==670);
  check(w.attack_duration(f.source,1000,200,true)==670);
  check(w.weapon_attack_timing(f.source,200)->period_ms==891);
  if(mode==0)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
  if(mode==1){check(w.shield_equipment(f.target,ShieldEquipment{1,42},8,100));check(w.status_mutation(f.target,378,124,12,200,5200,true));w.configure_attack_block_draws([]{return .74;});}
  if(mode==3){check(w.status_mutation(f.source,995,123,0,300,{},false));check(w.attack_duration(f.source,1000,300)==1000);continue;}
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1);
  check(outcomes[0].outcome==(mode==0?combat::weapon::AttackOutcome::miss:mode==1?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
  check(w.status_snapshot(f.source,300)->empty()==(mode!=2));
  check(w.attack_duration(f.source,1000,301)==(mode==2?670:1000));
  if(mode==2){check(w.attack_duration(f.source,1000,4100)==1000);}
 }
 for(unsigned mode=0;mode<3;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;
  check(w.status_mutation(f.source,995,123,0,100,4100,true));
  if(mode==0)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
  if(mode==1){check(w.shield_equipment(f.target,ShieldEquipment{1,42},8,100));check(w.status_mutation(f.target,378,124,12,200,5200,true));w.configure_attack_block_draws([]{return .74;});}
  check(w.ordinary_attack(f.source,2,200));w.tick(1000);
  const auto outcomes=w.take_attack_outcomes(f.source);check(!outcomes.empty());
  check(outcomes.front().outcome==(mode==0?combat::weapon::AttackOutcome::miss:mode==1?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
  check(w.status_snapshot(f.source,1000)->empty()==(mode!=2));
 }
 // Abandoned/projectile-retired attacks do not satisfy "fails to hit".
 {const Scope owner{1,1,1},target{1,2,1};Registry registry(owner);
  check(registry.apply(owner,{123,1,995,0,Category::stance,100,4100},Replace::category).accepted);
  for(const auto outcome:{combat::weapon::AttackOutcome::cancelled,combat::weapon::AttackOutcome::target_retired,combat::weapon::AttackOutcome::replaced})
   check(!tiger_stance_failed_hit(registry,owner,{owner,target,1,200,300,outcome}));
  check(bool(tiger_stance_failed_hit(registry,owner,{owner,target,1,200,300,combat::weapon::AttackOutcome::obstruction})));
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;casting::Runtime runtime(1);casting::Hooks h;
  std::vector<Packet> packets;h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy=[&](unsigned n){check(n==5);return w.consume_energy(1,n,f.source);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto until,bool active){return w.status_mutation(f.source,skill,effect,rank,at,until,active);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[995]=1;Slots bar{};bar[0]={995,0};attributes::Ranks ranks{};ranks[17]=0;
  check(runtime.start(1000,{995,0,0},bar,p,ranks,true,h)==casting::Reason::accepted);
  check(w.resources(1)->energy==25);
  check(w.status_snapshot(f.source,1000)->size()==1&&w.status_snapshot(f.source,1000)->front().expires==5000);
  check(std::find(packets.begin(),packets.end(),casting::generic(21,1,1271))!=packets.end());
  check(runtime.start(1001,{995,0,0},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Tiger Stance ranked lifetime, ordinary and skill attack speed, miss/block ending, hit retention, native instant cast/body and expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
