#include "gw_shields_up.h"
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Shields Up line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned type:{5u,22u,26u,35u})check(shields_up_projectile_weapon(type));
 for(unsigned type:{0u,2u,15u,27u})check(!shields_up_projectile_weapon(type));
 const Scope source{1,1,1},target{1,2,1};
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=shields_up(rank);check(p&&p->duration==1000ull*interpolate(5,11,rank)&&p->armor==60);
  const auto e=prepare_shields_up(*p,source,target,1,100,true,true,1012);check(e.has_value());
  Registry registry(target);check(registry.apply(target,*e,Replace::same_skill).accepted);
  check(!shields_up_armor(registry,target,99,ShieldsUpDamage::projectile_attack));
  check(shields_up_armor(registry,target,100,ShieldsUpDamage::projectile_attack)==60);
  check(shields_up_armor(registry,target,*e->expires-1,ShieldsUpDamage::projectile_attack)==60);
  for(auto kind:{ShieldsUpDamage::other,ShieldsUpDamage::melee_attack,ShieldsUpDamage::projectile_spell})check(!shields_up_armor(registry,target,100,kind));
  check(!shields_up_armor(registry,target,*e->expires,ShieldsUpDamage::projectile_attack));
  check(!shields_up_armor(registry,{1,2,2},100,ShieldsUpDamage::projectile_attack));
  auto duplicate=*e;duplicate.instance=2;check(registry.apply(target,duplicate,Replace::independent).accepted);
  check(shields_up_armor(registry,target,100,ShieldsUpDamage::projectile_attack)==60);
  registry.remove(target,1);registry.remove(target,2);check(!shields_up_armor(registry,target,100,ShieldsUpDamage::projectile_attack));
 }
 auto p=*shields_up(12);check(!shields_up(21));
 check(prepare_shields_up(p,source,source,1,100,false,true,0).has_value());
 check(!prepare_shields_up(p,source,target,1,100,false,true,0));
 check(!prepare_shields_up(p,source,target,1,100,true,false,0));
 for(float distance:{-1.f,1012.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})check(!prepare_shields_up(p,source,target,1,100,true,true,distance));
 check(!prepare_shields_up(p,source,{2,2,1},1,100,true,true,0));
 check(!prepare_shields_up(p,{1,1,0},target,1,100,true,true,0));
 check(!prepare_shields_up(p,source,target,0,100,true,true,0));
 check(!prepare_shields_up(p,source,target,1,UINT64_MAX,true,true,0));
 ++p.armor;check(!prepare_shields_up(p,source,target,1,100,true,true,0));
 p=*shields_up(12);Registry registry(target);auto e=*prepare_shields_up(p,source,target,1,100,true,true,0);++*e.expires;
 check(registry.apply(target,e,Replace::same_skill).accepted);check(!shields_up_armor(registry,target,100,ShieldsUpDamage::projectile_attack));
 std::cout<<"PASS Shields Up: 21 ranks, party/earshot admission, projectile attack armor, nonstacking, expiry, scope and corrupt-plan guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
