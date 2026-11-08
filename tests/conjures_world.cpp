#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Conjure World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{182u,207u,221u})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Elementalist",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,30,30},100);
  const auto owner=*w.effect_scope(1);const auto plan=*conjure(skill,12);
  check(w.conjure_enchantment(owner,plan,100));const auto before=*w.status_snapshot(owner,100);
  check(before.size()==1&&before[0].skill==skill&&before[0].rank==12&&before[0].cause==1&&before[0].expires==60100);
  check(!w.status_mutation(owner,skill,999,12,100,60100,true));
  auto forged=plan;++forged.damage;check(!w.conjure_enchantment(owner,forged,100));
  check(!w.conjure_enchantment(owner,plan,99)&&!w.conjure_enchantment(owner,plan,UINT64_MAX-100));
  const auto effect=before[0].instance;
  check(!w.status_mutation(owner,346,effect,0,100,8100,true));
  if(mode==0){w.tick(60099);check(w.status_snapshot(owner,60099)->size()==1);w.tick(60100);check(w.status_snapshot(owner,60100)->empty());}
  if(mode==1){check(w.status_mutation(owner,skill,effect,12,200,{},false));w.tick(60100);check(w.status_snapshot(owner,60100)->empty());}
  if(mode==2){check(w.conjure_enchantment(owner,*conjure(skill,20),200));const auto renewed=*w.status_snapshot(owner,200);check(renewed.size()==1&&renewed[0].instance!=effect&&renewed[0].rank==20&&renewed[0].expires==60200);
   w.tick(60100);check(w.status_snapshot(owner,60100)->size()==1);w.tick(60200);check(w.status_snapshot(owner,60200)->empty());}
  if(mode==3){check(w.kill(owner,200));check(w.status_snapshot(owner,200)->empty()&&!w.conjure_enchantment(owner,plan,200));}
  if(mode==4){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},200,1);w.set_vitals(1,{500,500,30,30},100);check(!w.conjure_enchantment(owner,plan,200)&&w.status_snapshot(*w.effect_scope(1),200)->empty());}
 }
 {
  world::Instance w;w.join(1,"Elementalist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,30,30},100);const auto owner=*w.effect_scope(1);
  for(unsigned skill:{182u,207u,221u})check(w.conjure_enchantment(owner,*conjure(skill,12),100));check(w.status_snapshot(owner,100)->size()==3);
 }
 std::cout<<"PASS Conjure World owned timers,rank,renewal,expiry,dispel,death,rejoin,coexistence and forged/timeline guards;attacks/casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
