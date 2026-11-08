#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
int main()try{
 bool passed=true;
 for(unsigned skill:{299u,2061u,258u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Actor",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto actor=*w.effect_scope(1);const auto plan=*skills::regeneration::reviewed(skill,12);
  box->drain();if(!w.protective_enchantment(actor,actor,plan,0,100))throw std::runtime_error("Protective application rejected");
  const auto packets=box->drain();
  const bool simple=std::count(packets.begin(),packets.end(),*skills::particles::target_body_2022(1,skill))==1;
  const bool targeted=std::find(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(1,1,skill))!=packets.end();
  if(!simple||targeted){passed=false;std::cerr<<"FAIL self body skill="<<skill<<" simple="<<simple<<" targeted="<<targeted<<'\n';}
 }
 if(!passed)return 1;
 std::cout<<"PASS protective self-cast packets match captured kind21forms exactly once;allied forms/rendering not claimed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
