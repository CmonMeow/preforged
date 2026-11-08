#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Attack secondary flesh line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{391u,404u,1719u})for(bool flesh:{false,true})for(bool enabled:{false,true})for(bool lethal:{false,true}){
  warrior_test::Fixture f(skill,true,lethal?1:1000,true,enabled,1,{},flesh);
  const auto kind=skill==404?combat::conditions::Kind::poison:combat::conditions::Kind::bleeding;
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12),1000));
  f.world.tick(1100);check(lethal?f.health()==0:f.health()<1000);
  check(f.world.has_condition(f.target,kind,1100)==(flesh&&enabled&&!lethal));
 }
 std::cout<<"24 NPC attack secondary flesh/condition-permission/lethal cases pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
