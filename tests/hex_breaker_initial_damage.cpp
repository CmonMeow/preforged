#include "gw_world_simulation.h"
#include <iostream>
using namespace fw::gw;
int main(){
 for(unsigned mode=0;mode<3;++mode){
  world::Instance w;
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Selected",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,100,100},100);w.set_vitals(2,{mode==2?20u:500u,500,100,100},100);
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2);
  if(!w.elemental_armor(owner,60,60))return 2;
  if(mode!=1&&!w.apply_condition(caster,owner,combat::conditions::Kind::blind,100,20000))return 3;
  if(!w.hex_breaker(owner,*skills::effects::hex_breaker(12),20,100))return 4;
  if(!w.condition_damage(caster,owner,*skills::effects::condition_damage(828,12),20,200))return 5;
  const auto effects=*w.status_snapshot(owner,200);
  const bool stance=std::any_of(effects.begin(),effects.end(),[](const auto& e){return e.skill==10;});
  const bool hex=std::any_of(effects.begin(),effects.end(),[](const auto& e){return e.skill==828;});
  if(hex||stance!=(mode==1)||w.resources(1)->health!=(mode==0?461u:500u)||w.resources(2)->health!=(mode==2?0u:446u)){
   std::cerr<<"FAIL initial damage/prevention mode "<<mode<<'\n';return 1;
  }
 }
 std::cout<<"PASS conditional damaging hex retains initial damage; absent hex and lethal initial damage do not retaliate\n";
}
