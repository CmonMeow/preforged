#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Berserker Stance World line "+std::to_string(at.line()));}
combat::adrenaline::Pool pool(world::Instance& w,Scope scope){const auto v=w.adrenaline(scope);check(v.has_value());for(const auto& p:*v)if(p.slot.skill==340)return p;throw std::runtime_error("missing pool");}
void apply(world::Instance& w,Scope scope){Slots bar{};bar[0]={340,0};bar[1]={370,0};check(w.configure_adrenaline(scope,bar,100));check(w.status_mutation(scope,370,123,0,100,5100,true));}
int main()try{
 for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;apply(w,f.source);
  check(w.weapon_attack_timing(f.source,200)->period_ms==891);check(w.weapon_attack_timing(f.source,200,true)->period_ms==1330);
  check(w.attack_duration(f.source,500,200)==335&&w.attack_duration(f.source,500,200,true)==500);
  if(mode==1)check(w.status_mutation(f.source,56,124,0,100,8100,true));
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==3)check(w.status_mutation(f.source,370,123,0,200,{},false));
  if(mode==4)check(w.kill(f.source,200));
  if(mode==4){check(!w.ordinary_attack(f.source,2,200));continue;}
  check(w.ordinary_attack(f.source,2,200));w.tick(mode==3?700:535);
  auto p=pool(w,f.source);check(p.units==unsigned(mode==1||mode==2?0:mode==3?25:37)&&p.half==unsigned(mode==0));
  if(mode==0){w.tick(1535);p=pool(w,f.source);check(p.units==75&&!p.half);}
 }
 for(bool forbidden:{false,true}){
  warrior_test::Fixture f(340,false);auto& w=f.world;apply(w,f.target);
  if(forbidden)check(w.status_mutation(f.target,56,124,0,100,8100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));
  check(pool(w,f.target).units==unsigned(forbidden?0:3));
 }
 // Stance ends for the next accepted instant skill;it is not a timing bonus
 // for attack skills. Backbreaker's pre-payment permit uses that same rule.
 {warrior_test::Fixture f(359,false);auto& w=f.world;Slots bar{};bar[0]={359,0};bar[1]={370,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,370,123,0,100,5100,true));
  const auto permit=w.prepare_melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(359,12),1000);
  check(permit&&permit->release_time()==1500);check(w.accepted_skill_start(f.source,{359,0},1000));
  check(w.attack_duration(f.source,500,1000)==500);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={370,0};bar[1]={344,0};check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> packets;unsigned calls=0;
  h.events=[&](const auto& b){packets.insert(packets.end(),b.begin(),b.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](auto n){check(n==5);return w.consume_energy(1,n,f.source);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto rank,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,rank,at,until,active);};
  h.begin_action=[&](Slot used,auto at){++calls;check(w.accepted_skill_start(f.source,used,at));};
  Policy p;p.primary=1;p.campaigns=255;p.learned[370]=p.learned[344]=1;attributes::Ranks ranks{};ranks[17]=12;
  check(runtime.start(1000,{370,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(w.resources(1)->energy==25&&calls==1);
  const auto active=*w.status_snapshot(f.source,1000);check(active.size()==1&&active[0].skill==370&&active[0].expires==11000);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,629))==1);
  check(runtime.start(1001,{370,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
  check(runtime.start(2000,{344,0,1},bar,p,ranks,true,h)==casting::Reason::accepted&&calls==2);
  const auto after=*w.status_snapshot(f.source,2000);check(after.size()==1&&after[0].skill==344);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;apply(w,f.source);
  check(w.weapon_attack_timing(f.source,5100)->period_ms==1330);
 }
 std::cout<<"PASS Berserker Stance ordinary IAS and exact fractional hit/damage adrenaline,miss/forbidden/removal/death,prospective skill/Backbreaker timing,Runtime Energy/recharge/body629 and accepted instant ending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
