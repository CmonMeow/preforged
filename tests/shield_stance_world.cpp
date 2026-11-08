#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include "gw_shadow_damage.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Shield Stance World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<8;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==0?.74:.99;});
  check(w.shield_equipment(f.target,ShieldEquipment{1,42},8,100));
  check(w.status_mutation(f.target,378,123,12,100,5100,true));
  if(mode==2)check(w.shield_equipment(f.target,ShieldEquipment{2,0},8,200));
  if(mode==3)check(w.shield_equipment(f.target,ShieldEquipment{2,42},0,200));
  if(mode==4)check(w.status_mutation(f.target,371,124,0,200,8200,true));
  if(mode==5)check(w.status_mutation(f.target,378,123,12,200,{},false));
  if(mode==7){check(w.shield_equipment(f.target,ShieldEquipment{2,0},8,200));check(w.shield_equipment(f.target,ShieldEquipment{3,42},3,250));}
  if(mode==6){check(w.shield_equipment(f.target,{}, {},200));check(!w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));check(f.health()==1000);continue;}
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  if(mode==0){check(f.health()==1000&&draws==1);check(w.take_attack_outcomes(f.source).front().outcome==combat::weapon::AttackOutcome::block);}
  else check(f.health()==unsigned(mode==1?995:mode==7?986:980));
  if(mode>=2)check(draws==unsigned(mode==3||mode==7?1:0));
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;
  check(w.shield_equipment(f.target,ShieldEquipment{10,42},8,100));
  check(!w.shield_equipment(f.target,ShieldEquipment{9,42},8,100));
  auto stale=f.target;++stale.generation;check(!w.shield_equipment(stale,ShieldEquipment{11,42},8,100));
  check(!w.shield_equipment(f.target,ShieldEquipment{11,42},21,100));
  check(w.status_mutation(f.target,378,123,0,100,1100,true));
  check(!w.shield_equipment(f.target,ShieldEquipment{11,0},8,99));
  w.configure_attack_block_draws([]{return .99;});check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),1100));check(f.health()==980);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;
  check(w.shield_equipment(f.target,ShieldEquipment{1,42},8,100));check(w.status_mutation(f.target,378,123,12,100,5100,true));
  check(w.status_mutation(f.target,137,124,0,100,8100,true));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));check(f.health()==995&&draws==0);
 }
 // Production Runtime owns cost,recharge,self-targeting,registry and native body.
 {warrior_test::Fixture f(340,false);auto& w=f.world;casting::Runtime runtime(1);casting::Hooks h;
  unsigned spent=0;std::vector<items::Packet> packets;h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy=[&](auto n){spent+=n;return w.consume_energy(1,n,f.source);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(f.source,skill,effect,rank,now,until,active);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[378]=1;Slots bar{};bar[0]={378,0};attributes::Ranks ranks{};ranks[21]=12;
  check(runtime.start(1000,{378,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(spent==5);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,637))==1);
  const auto entries=*w.status_snapshot(f.source,1000);check(entries.size()==1&&entries[0].skill==378&&entries[0].expires==6000);
  check(runtime.start(1001,{378,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
  check(w.kill(f.source,1100));check(w.status_snapshot(f.source,1100)->empty());
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;const auto plan=*shadow_damage(102,12);
  check(w.shield_equipment(f.target,ShieldEquipment{1,42},8,100));check(w.status_mutation(f.target,378,123,12,100,5100,true));
  check(w.shadow_damage(f.source,f.target,plan,20,200));check(f.health()==1000-plan.damage+15-plan.steal_above_half);
 }
 // Combined direct damage and life stealing use distinct protection channels.
 {combat::Actor target{2,1,combat::Health{1000,1000},{},0,true};target.incoming_reduction=15;target.life_steal_reduction=0;
  const auto plan=*shadow_damage(102,12);const auto hit=shadow_target(plan,{100,1000},target,20);check(hit&&hit->steal);
  check(hit->hit.damage==plan.damage-15&&hit->steal->stolen==plan.steal_above_half);
 }
 std::cout<<"PASS Shield Stance World block/reduction,current gear/Strength updates,expiry/replacement/removal,unknown/stale facts,Rigor bypass,Runtime cost/recharge/death and separate life-steal protection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
