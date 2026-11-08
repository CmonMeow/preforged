#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Diversion use World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<8;++mode){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Victim",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2);auto selected=owner;
  if(mode==4){check(w.diversion_skill_used(owner,{28,0},7,100,20000,0)==0);}
  check(w.diversion(caster,owner,*skills::effects::diversion(12),100));
  if(mode==2)++selected.generation;
  const skills::Slot used{mode==1?22u:28u,0};
  if(mode==5){
   check(!w.diversion_skill_used(owner,used,7,200,86400001,0));
   check(w.diversion_skill_used(owner,used,7,200,20000,0)==67200);continue;
  }
  if(mode==6){
   check(w.diversion_skill_used(owner,used,7,6200,20000,0)==0);
   check(w.status_snapshot(owner,6200)->empty());check(w.diversion_recharge_until(owner,used,6200)==0);continue;
  }
  if(mode==7){
   check(w.diversion_skill_used(owner,used,7,200,20000,0)==67200);
   w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},300,2);w.set_vitals(2,{500,500,100,100},100);
   const auto replacement=*w.effect_scope(2);check(replacement!=owner);
   check(!w.diversion_recharge_until(owner,used,300));check(w.diversion_recharge_until(replacement,used,300)==0);
   check(w.diversion(caster,replacement,*skills::effects::diversion(12),300));
   check(w.diversion_skill_used(replacement,used,7,400,20000,0)==67400);continue;
  }
  const auto until=w.diversion_skill_used(selected,used,7,200,20000,mode==3?UINT64_MAX:0);
  if(mode==2||mode==4){check(!until);const auto effects=*w.status_snapshot(owner,200);check(std::any_of(effects.begin(),effects.end(),[](const auto& e){return e.skill==30;}));continue;}
  check(until&&*until==(mode==1?0u:mode==3?UINT64_MAX:67200u));
  check(w.status_snapshot(owner,200)->empty());
  check(w.diversion_recharge_until(owner,used,200)==*until);
  check(!w.diversion_skill_used(owner,used,7,201,20000,0));
  if(mode==0)check(w.diversion_recharge_until(owner,used,67200)==0);
 }
 {
  world::Instance w;w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);
  check(!w.diversion_skill_used(owner,{28,0},7,200,86400001,0));
  check(w.diversion_skill_used(owner,{28,0},7,200,20000,0)==0);
 }
 std::cout<<"PASS World Diversion one-use authority, preserved long deadline, invalid-base rollback, expiry, stale/replay and retired-actor isolation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
