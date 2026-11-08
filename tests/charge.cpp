#include "gw_charge.h"
#include "gw_movement_effects.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Charge plan line "+std::to_string(at.line()));}
int main()try{
 const Scope source{1,1,1},target{1,2,1};
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=*charge(rank);check(plan.duration==1000ull*unsigned(std::lround(5.+8.*rank/15.))&&plan.energy==5&&plan.recharge==20000&&plan.range==1012);
  const auto entry=*prepare_charge(plan,source,target,10,1000);check(entry.cause==1&&entry.kind==Category::other&&entry.expires==1000+plan.duration);
  Registry registry(target);check(registry.apply(target,entry,Replace::same_skill).accepted);
  MovementEffects speed;speed.apply(entry);check(speed.at(999)==1.f&&speed.at(1000)==1.33f&&speed.at(1000+plan.duration)==1.f);
  check(std::abs(speed.integral(500,1500+plan.duration)-(1000.+plan.duration*double(1.33f)))<.01);
  speed.remove(10,1500);check(speed.at(1499)==1.33f&&speed.at(1500)==1.f);
  combat::conditions::State conditions;check(conditions.apply(combat::conditions::Kind::crippled,3,100,10000)&&conditions.apply(combat::conditions::Kind::poison,3,100,10000));
  const auto proposal=*charge_cleanse(plan,conditions,1000);check(proposal.removed&&conditions.has(combat::conditions::Kind::crippled,1000));
  check(!proposal.conditions.has(combat::conditions::Kind::crippled,1000)&&proposal.conditions.has(combat::conditions::Kind::poison,1000));
  auto fresh=proposal.conditions;check(fresh.apply(combat::conditions::Kind::crippled,3,1200,3000));MovementEffects combined;combined.apply(entry);combined.crippled(fresh,1200);
  check(combined.at(1199)==1.33f&&std::abs(combined.at(1200)-.665f)<.0001); // No persistent immunity.
  MovementEffects cap;cap.apply(entry);cap.apply({11,1,37,0,Category::enchantment,1000,5000});check(cap.at(1000)==1.34f);
  auto renewed=*prepare_charge(plan,source,target,12,2000);MovementEffects renewal;renewal.apply(entry);renewal.apply(renewed);check(renewal.at(2000)==1.33f);
  auto forged=plan;++forged.energy;check(!prepare_charge(forged,source,target,10,1000)&&!charge_cleanse(forged,conditions,1000));
  check(!prepare_charge(plan,source,{2,2,1},10,1000)&&!prepare_charge(plan,source,target,0,1000)&&!prepare_charge(plan,source,target,10,UINT64_MAX));
 }
 check(!charge(21));auto plan=*charge(0);combat::conditions::State none;check(!charge_cleanse(plan,none,0)->removed);
 // Preserve the already-travelled slow segment when cleansing between ticks.
 combat::conditions::State crippled;check(crippled.apply(combat::conditions::Kind::crippled,3,100,10000));
 MovementEffects cleansed;cleansed.crippled(crippled,100);
 const auto proposal=*charge_cleanse(plan,crippled,1000);cleansed.crippled(proposal.conditions,1000);cleansed.apply(*prepare_charge(plan,source,target,20,1000));
 check(cleansed.at(999)==.5f&&cleansed.at(1000)==1.33f&&std::abs(cleansed.integral(100,1500)-(450.+500.*double(1.33f)))<.01);
 MovementEffects wrong;wrong.apply({1,1,364,0,Category::enchantment,100,5100});check(wrong.at(100)==1.f);wrong.apply({1,1,364,21,Category::other,100,5100});check(wrong.at(100)==1.f);
 std::cout<<"PASS Charge21ranks,owned status,initial Crippled-only cleanse proposal,speed intervals/expiry/removal/renewal/cap,new Crippled and invalid plans;World/casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
