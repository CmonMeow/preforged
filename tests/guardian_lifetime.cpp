#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Guardian lifetime regression line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<3;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Attacker",0,0,box,0,{}, {},100,1);w.join(2,"Defender",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto source=*w.effect_scope(1),old_target=*w.effect_scope(2);
  const auto plan=*skills::regeneration::reviewed(258,12);
  check(w.apply_health_regeneration(old_target,2,plan,100,6100,1248,true,old_target));
  const auto original=w.status_snapshot(old_target,100)->front();
  if(mode==0)check(w.status_mutation(old_target,258,original.instance,12,200,{},false));
  if(mode==1){
   w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},200,2);w.set_vitals(2,{500,500,100,100},100);
   check(*w.effect_scope(2)!=old_target);
   check(!w.apply_health_regeneration(old_target,2,plan,200,6200,1248,true,old_target));
  }
  if(mode==2){
   check(w.apply_health_regeneration(old_target,2,plan,200,6200,1248,true,old_target));
   const auto renewed=*w.status_snapshot(old_target,200);check(renewed.size()==1&&renewed[0].instance!=original.instance);
  }
  const auto target=*w.effect_scope(2);check(w.physical_armor(target,60));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
  combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(source,sword,20));
  const unsigned at=mode==2?6100:300;
  check(w.melee_attack(source,target,sword,*combat::weapon::attack_skill(322,0),at));
  check(w.resources(2)->health==(mode==2?500u:470u));
  check(draws==(mode==2?1u:0u));
  const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==1);
  check(outcomes[0].outcome==(mode==2?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
 }
 std::cout<<"PASS Guardian removal and defender replacement clear block;stale source rejected;renewal replaces timer and remains active beyond old expiry\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
