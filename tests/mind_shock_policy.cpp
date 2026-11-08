#include "gw_mind_shock.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <tuple>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Mind Shock policy line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills::effects;constexpr std::int64_t unit=300000;
 const combat::Actor base{2,1,combat::Health{1000,1000},{},0,true,80};
 combat::conditions::State conditions;
 for(unsigned skill:{226u,2804u})for(unsigned rank=0;rank<=20;++rank)for(bool bonus:{false,true}){
  const auto plan=mind_shock(skill,rank);check(plan&&plan->damage==damage_scale(10,50,rank)&&plan->penetration==25&&plan->knockdown_ms==2000);
  const auto result=mind_shock_target(*plan,base,conditions,20,bonus?6*unit:5*unit,5*unit,1000);
  check(result.accepted&&result.energy_advantage==bonus&&result.knockdown==bonus&&result.hits.size()==(bonus?2:1));
  check(std::abs(result.actor.health->current-(1000-plan->damage*(bonus?2:1)))<1e-7);
 }
 const auto plan=*mind_shock(226,12);check(!mind_shock(226,21)&&!mind_shock(222,12));
 for(auto [source,target,bonus]:{std::tuple{5*unit,5*unit,false},std::tuple{5*unit+1,5*unit,true},
      std::tuple{5*unit-1,5*unit,false},std::tuple{0ll,-1ll,true},std::tuple{-2ll,-1ll,false}}){
  const auto result=mind_shock_target(plan,base,conditions,20,source,target,1000);
  check(result.accepted&&result.energy_advantage==bonus&&result.hits.size()==(bonus?2:1));
 }
 for(unsigned hp:{1u,42u,43u,84u,85u}){
  auto actor=base;actor.health->current=hp;const auto result=mind_shock_target(plan,actor,conditions,20,6*unit,5*unit,1000);
  check(result.accepted&&result.hits.size()==(hp<=42?1:2)&&result.knockdown==(hp>84));
  check(result.actor.health->current==std::max(0.,double(hp)-84));
 }
 {
  auto actor=base;actor.incoming_reduction=10;
  const auto result=mind_shock_target(plan,actor,conditions,20,6*unit,5*unit,1000);
  check(result.accepted&&result.hits.size()==2&&result.hits[0].applied==32&&result.hits[1].applied==32&&result.actor.health->current==936);
 }
 {
  auto cracked=conditions;check(cracked.apply(combat::conditions::Kind::cracked_armor,1,500,5000));
  const auto result=mind_shock_target(plan,base,cracked,20,6*unit,5*unit,1000);
  check(result.accepted&&std::abs(result.actor.health->current-(1000-84*std::exp2(15./40.)))<1e-7);
 }
 check(!mind_shock_target(plan,base,conditions,20,{},5*unit,1000).accepted);
 check(!mind_shock_target(plan,base,conditions,20,6*unit,{},1000).accepted);
 auto forged=plan;forged.damage=999;check(!mind_shock_target(forged,base,conditions,20,6*unit,5*unit,1000).accepted);
 auto unknown=base;unknown.lightning_armor.reset();check(!mind_shock_target(plan,unknown,conditions,20,6*unit,5*unit,1000).accepted);
 check(!mind_shock_target(plan,base,conditions,0,6*unit,5*unit,1000).accepted);
 std::cout<<"PASS Mind Shock PvE/PvP rank policy,signed precise Energy threshold,separate-hit mitigation and lethal control eligibility\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
