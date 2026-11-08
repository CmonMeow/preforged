#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Wary World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;
  skills::Slots defender_bar{};defender_bar[0]={340,0};check(w.configure_adrenaline(f.target,defender_bar,100));
  w.set_vitals(2,{1000,1000,20,30},100);
  check(w.status_mutation(f.target,377,123,0,100,1100,true));w.configure_attack_block_draws([]{return .99;});
  if(mode==2)check(w.status_mutation(f.target,137,124,0,100,8100,true));
  if(mode==4)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==1){check(w.ordinary_attack(f.source,2,200));w.tick(700);check(f.health()<1000);}
  else{check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),mode==3?1100:300));
   check((f.health()==1000)==(mode==0||mode==4));
   const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1);
   check(outcomes[0].outcome==(mode==0?combat::weapon::AttackOutcome::block:mode==4?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
  }
  check(w.resources(2)->energy==unsigned(mode==0?25:20));
  if(mode==0||mode==4)check(w.adrenaline(f.target)->front().units==unsigned(mode==0?35:0));
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;w.set_vitals(2,{1000,1000,20,30},100);
  skills::Slots defender_bar{};defender_bar[0]={340,0};check(w.configure_adrenaline(f.target,defender_bar,100));
  check(w.status_mutation(f.target,377,123,12,100,5100,true));w.configure_attack_block_draws([]{return .99;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  check(w.resources(2)->energy==25);
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),400));
  check(w.resources(2)->energy==30);
  check(w.adrenaline(f.target)->front().units==70);
  check(w.status_mutation(f.target,377,123,0,500,{},false));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),600));
  check(w.resources(2)->energy==30);
 }
 {warrior_test::Fixture f(399,false);auto& w=f.world;w.set_vitals(2,{1000,1000,20,30},100);w.configure_attack_block_draws([]{return .99;});
  skills::Slots defender_bar{};defender_bar[0]={340,0};check(w.configure_adrenaline(f.target,defender_bar,100));
  check(w.status_mutation(f.target,377,123,0,100,1100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);check(f.health()==1000&&w.resources(2)->energy==25);
  check(w.adrenaline(f.target)->front().units==35);
 }
 {warrior_test::Fixture f(330,false);auto& w=f.world;w.set_vitals(2,{1000,1000,20,30},100);w.configure_attack_block_draws([]{return .99;});
  skills::Slots defender_bar{};defender_bar[0]={340,0};check(w.configure_adrenaline(f.target,defender_bar,100));
  check(w.status_mutation(f.target,377,123,0,100,1100,true));
  check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12),300));check(f.health()==1000&&w.resources(2)->energy==25);
  check(w.adrenaline(f.target)->front().units==35);
 }
 std::cout<<"PASS Wary qualified melee/ranged/Cyclone blocks and final player block Energy/35 adrenaline; ordinary/Rigor/miss/expiry exclusion\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
