#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Wild Blow accepted use line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<8;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={321,0};bar[1]={335,0};bar[2]={335,1};
  check(w.configure_adrenaline(f.source,bar,100));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  for(const auto& p:*w.adrenaline(f.source))check(p.units==50);
  if(mode==1)check(w.status_mutation(f.source,370,123,0,300,5300,true));
  if(mode==2)check(w.status_mutation(f.source,380,123,0,300,5300,true));
  if(mode==3)check(w.status_mutation(f.source,377,123,0,300,1300,true));
  if(mode==7)check(w.configure_adrenaline(f.source,bar,500));
  auto scope=f.source;if(mode==4)++scope.generation;
  const Slot used{321,mode==5?1u:0u};
  check(w.accepted_skill_start(scope,used,mode==6?299:400)==(mode<4));
  for(const auto& p:*w.adrenaline(f.source))check(p.units==(mode<4?0:50));
  if(mode>0&&mode<4)check(w.status_snapshot(f.source,400)->empty());
 }
 std::cout<<"PASS staged Wild Blow accepted-use all-pool/copy loss,exact owner/copy validation and Berserker/Bonetti/Wary combined ending;Runtime binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
