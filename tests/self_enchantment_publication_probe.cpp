#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Self enchant publication line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<2;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Renewal",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,50,100},100);auto owner=*w.effect_scope(1);
  const auto er=*ether_renewal(181,0);check(w.status_mutation(owner,181,80,0,100,100+er.duration,true));
  if(mode)check(w.apply_condition(owner,owner,combat::conditions::Kind::burning,100,5000));
  check(w.before_spell_effect(owner,1375,1,200));box->drain();const auto before=*w.resources(1);
  std::function<void()> publication;std::uint64_t at=0;
  const bool ok=w.stoneflesh_aura_enchantment(owner,0,mode?1200:200,mode?2:1,nullptr,&at,&publication);
  if(!mode){check(ok&&bool(publication)&&at==200);check(w.resources(1)->health==before.health+5&&w.resources(1)->energy==before.energy+1);
   check(box->drain().empty());publication();check(!box->drain().empty());publication();check(box->drain().empty());
   publication={};check(!w.stoneflesh_aura_enchantment(owner,0,200,1,nullptr,&at,&publication)&&!publication&&box->drain().empty());
  }else{check(!ok&&!publication);check(w.resources(1)->health<before.health);check(!box->drain().empty());const auto statuses=*w.status_snapshot(owner,1200);check(std::none_of(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==1375;}));}
 }
 std::cout<<"PASS owned self enchant captures Renewal before publication; refusal flushes causal settlement\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
