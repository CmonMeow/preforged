#include "gw_echo.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok){if(!ok)throw std::runtime_error("Echo completed-use proposal regression");}
int main()try{
 const Scope owner{1,2,3};
 for(unsigned skill:{74u,75u}){
  Registry registry(owner);const auto plan=*echo(skill);
  check(registry.apply(owner,{1,2,skill,0,Category::enchantment,100,100+plan.armed_duration},Replace::same_skill).accepted);
  const auto copied=prepare_echo_completed_use(registry,owner,{skill,0},{76,0},1000);
  check(copied&&copied->copied_skill==76&&copied->copy_duration==(skill==74?30000:20000)&&copied->restore_cooldown==(skill==74?10000:20000));
  check(copied->restoration==(skill==74?EchoRestorationKind::observed_cooldown:EchoRestorationKind::independent_disable));
  check(registry.lookup(owner,1).has_value()&&!copied->registry.lookup(owner,1)); // Proposal cannot mutate live arm.
  check(!prepare_echo_completed_use(copied->registry,owner,{skill,0},{76,0},1000));
  check(!prepare_echo_completed_use(registry,{1,2,4},{skill,0},{76,0},1000));
  check(!prepare_echo_completed_use(registry,owner,{skill,0},{skill,1},1000));
  check(!prepare_echo_completed_use(registry,owner,{skill,0},{76,0},99));
  check(!prepare_echo_completed_use(registry,owner,{skill,0},{76,0},100+plan.armed_duration));
  const auto signet=prepare_echo_completed_use(registry,owner,{skill,0},{1,0},1000);
  check(bool(signet));
  if(skill==74)check(signet->copied_skill==1&&signet->copy_duration==30000);
  else check(!signet->copied_skill&&!signet->copy_duration&&!signet->restore_cooldown);
  const auto nested=prepare_echo_completed_use(registry,owner,{skill,0},{skill==74?75u:74u,1},1000);
  check(nested&&nested->copied_skill==(skill==74?75u:74u)); // Cross-Echo chains remain possible.
 }
 Registry registry(owner);check(registry.apply(owner,{1,2,74,0,Category::enchantment,100,30100},Replace::same_skill).accepted);
 check(!prepare_echo_completed_use(registry,owner,{74,0},{3,0},1000));
 check(!prepare_echo_completed_use(registry,owner,{74,0},{99999,0},1000));
 std::cout<<"PASS Echo/Arcane Echo scoped consumable proposals,spell/nonspell differences,exclusions,copy duration and restoration disable;session/runtime integration pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
