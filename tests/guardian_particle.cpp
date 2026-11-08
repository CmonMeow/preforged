#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
int main()try{
 world::Instance world;auto box=std::make_shared<world::Outbox>();
 world.join(1,"Actor",0,0,box,0,{}, {},100,1);world.set_vitals(1,{500,500,100,100},100);
 const auto actor=*world.effect_scope(1);
 const auto plan=*skills::regeneration::reviewed(258,12);
 box->drain();
 if(!world.apply_health_regeneration(actor,1,plan,100,6100,1248,true,actor))throw std::runtime_error("Guardian application failed");
 const auto packets=box->drain();
 if(std::count(packets.begin(),packets.end(),*skills::particles::target_body_2022(1,258))!=1)
  throw std::runtime_error("Guardian must publish captured kind21 body472 exactly once");
 if(std::find(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(1,1,258))!=packets.end())
  throw std::runtime_error("Guardian must not substitute unobserved targeted kind20 body");
 std::cout<<"PASS Guardian World packet matches captured kind21body472;rendered effect remains unverified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
