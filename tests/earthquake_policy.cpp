#include "gw_earthquake.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earthquake policy line "+std::to_string(at.line()));
}
combat::Actor body(){combat::Actor a;a.id=2;a.generation=3;a.health=combat::Health{1000,1000};a.earth_armor=80;a.damageable=true;return a;}
int main()try{
 check(!earthquake(169,12)&&!earthquake(170,21));
 for(unsigned skill:{170u,1086u})for(unsigned rank:{0u,12u,20u})for(bool immune:{false,true}){
  const auto plan=*earthquake(skill,rank);const auto actor=body();
  const auto hit=earthquake_target(plan,actor,{},20,100,!immune);
  const auto damage=damage_scale(26,100,rank)*std::exp2(-.5);
  check(hit&&std::abs(hit->hit.damage-damage)<1e-9&&hit->knockdown==!immune);
  check(actor.health->current==1000&&std::abs(hit->actor.health->current-(1000-damage))<1e-9);
  check(!earthquake_target(plan,actor,{},20,100,std::nullopt));
  auto forged=plan;forged.radius=166;check(!earthquake_target(forged,actor,{},20,100,true));
 }
 auto actor=body();auto plan=*earthquake(1086,12);combat::conditions::State conditions;
 check(conditions.apply(combat::conditions::Kind::cracked_armor,1,100,1000));
 auto hit=earthquake_target(plan,actor,conditions,20,200,true);check(hit&&std::abs(hit->hit.damage-85.2)<1e-9);
 actor.health->current=1;hit=earthquake_target(plan,actor,{},20,100,std::nullopt);
 check(hit&&hit->hit.killed&&!hit->knockdown); // Corpse needs no control authority.
 actor=body();actor.earth_armor.reset();check(!earthquake_target(plan,actor,{},20,100,true));
 actor=body();actor.damageable=false;check(!earthquake_target(plan,actor,{},20,100,true));
 std::cout<<"PASS Earthquake/Dragon's Stomp canonical metadata,rank/armor,immunity vs unknown control,lethal suppression and proposal isolation; World behavior tested separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
