#include "gw_thunderclap.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Thunderclap policy line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills::effects;using combat::conditions::Kind;
 const combat::Actor base{2,1,combat::Health{1000,1000},{},0,true,80};
 const combat::conditions::State clean;
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=thunderclap(rank);check(plan&&plan->damage==damage_scale(10,50,rank)&&plan->condition_ms==(5+rank)*1000);
  const auto r=thunderclap_target(*plan,base,clean,20,1,1000);
  check(r.accepted&&r.hit.applied==plan->damage&&r.actor.health->current==1000-plan->damage);
  check(r.applied_conditions==std::vector<Kind>{Kind::cracked_armor,Kind::weakness});
  check(r.conditions.newest(1000)==Kind::weakness);
  check(r.conditions.lookup(Kind::cracked_armor)->cause==1);
  check(r.conditions.has(Kind::weakness,999+plan->condition_ms)&&!r.conditions.has(Kind::weakness,1000+plan->condition_ms));
  auto stripped=r.conditions;check(stripped.remove(*stripped.newest(1000)));
  check(stripped.newest(1000)==Kind::cracked_armor);
 }
 const auto plan=*thunderclap(12);check(!thunderclap(21));
 {
  auto previous=clean;check(previous.apply(Kind::cracked_armor,9,500,40000));
  auto r=thunderclap_target(plan,base,previous,20,1,1000);
  check(r.accepted&&std::abs(r.hit.applied-42*std::exp2(15./40.))<1e-7);
  check(r.conditions.lookup(Kind::cracked_armor)->expires==40500&&r.conditions.newest(1000)==Kind::weakness);
 }
 for(bool immune:{false,true}){
  auto r=thunderclap_target(plan,base,clean,20,1,1000,!immune);
  check(r.accepted&&r.hit.applied==42&&r.conditions.count(1000)==(immune?0:2));
 }
 for(unsigned hp:{1u,42u,43u}){
  auto a=base;a.health->current=hp;auto r=thunderclap_target(plan,a,clean,20,1,1000);
  check(r.accepted&&r.hit.killed==(hp<=42)&&r.conditions.count(1000)==(hp<=42?0:2));
 }
 auto forged=plan;forged.radius=252;check(!thunderclap_target(forged,base,clean,20,1,1000).accepted);
 auto unknown=base;unknown.lightning_armor.reset();check(!thunderclap_target(plan,unknown,clean,20,1,1000).accepted);
 check(!thunderclap_target(plan,base,clean,0,1,1000).accepted);
 check(!thunderclap_target(plan,base,clean,20,0,1000).accepted);
 check(!thunderclap_target(plan,base,clean,20,1,UINT64_MAX-1).accepted);
 std::cout<<"PASS Thunderclap prepared recipient policy: damage before ordered conditions,cover removal,immunity,lethality and expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
