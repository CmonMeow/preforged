#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Diversion World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Target",mode==4?2000.f:60.f,0,box,0,{}, {},100,mode==5?1:2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1);auto target=*w.effect_scope(2);auto plan=*skills::effects::diversion(12);
  if(mode==1)check(w.diversion(caster,target,plan,100));
  if(mode==2)check(w.hex_breaker(target,*skills::effects::hex_breaker(12),20,100));
  if(mode==3)++target.generation;if(mode==6)++plan.duration;
  box->drain();check(w.diversion(caster,target,plan,200)==(mode<3));
  if(mode<3){const auto effects=*w.status_snapshot(target,200);
   check(std::count_if(effects.begin(),effects.end(),[](const auto& e){return e.skill==30;})==(mode==2?0:1));
   check(w.resources(1)->health==(mode==2?461u:500u));
   const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,30))==(mode==2?0:1));
   if(mode!=2){w.tick(6200);const auto after=*w.status_snapshot(target,6200);check(std::none_of(after.begin(),after.end(),[](const auto& e){return e.skill==30;}));}
  }
 }
 std::cout<<"PASS candidate Diversion World apply/renew/expiry, body52, Hex Breaker and scope/range/team/plan guards; trigger/casting pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
