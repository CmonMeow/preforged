#include "gw_defensive_ground_ward.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Defensive ward line "+std::to_string(at.line()));
}
combat::Packet bytes(std::string_view s){combat::Packet p;for(std::size_t i=0;i<s.size();i+=2)p.push_back(std::uint8_t(std::stoul(std::string(s.substr(i,2)),nullptr,16)));return p;}
int main()try{unsigned cases=0;
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=defensive_ground_ward_plan(938,rank);check(plan&&plan->duration==1000ull*(10+rank));
  const auto ward=defensive_ground_ward({1,25,1},80,2,0,0,0,*plan,100);check(ward.has_value());
  DefensiveWardRecipient r{{1,30,1},2,0,322,0,true,false};
  check(defensive_ward_prevents_knockdown(*ward,r,100));check(defensive_ward_prevents_knockdown(*ward,r,ward->expires-1));
  check(!defensive_ward_prevents_knockdown(*ward,r,99)&&!defensive_ward_prevents_knockdown(*ward,r,ward->expires));
  r.x=std::nextafter(322.f,1000.f);check(!defensive_ward_prevents_knockdown(*ward,r,100));r.x=0;
  r.spirit=true;check(!defensive_ward_prevents_knockdown(*ward,r,100));r.spirit=false;r.living=false;check(!defensive_ward_prevents_knockdown(*ward,r,100));r.living=true;
  r.team=1;check(!defensive_ward_prevents_knockdown(*ward,r,100));r.team=2;r.plane=1;check(!defensive_ward_prevents_knockdown(*ward,r,100));r.plane=0;
  r.scope.instance=2;check(!defensive_ward_prevents_knockdown(*ward,r,100));r.scope.instance=1;
  // An area protects a fresh allied body at its position, not ownership of
  // the old caster's personal timer. Original cast scope remains unchanged.
  r.scope={1,25,2};check(defensive_ward_prevents_knockdown(*ward,r,100)&&ward->caster==Scope{1,25,1});
  r.x=500;check(!defensive_ward_prevents_knockdown(*ward,r,100));r.x=0;
  check(defensive_ward_prevents_knockdown(*ward,r,100));
  auto forged=*ward;forged.expires++;check(!defensive_ward_prevents_knockdown(forged,r,100));
  auto modified=*plan;++modified.radius;check(!defensive_ground_ward({1,25,1},80,2,0,0,0,modified,100));++cases;
 }
 check(!defensive_ground_ward_plan(938,21)&&!defensive_ground_ward_plan(175,0));
 const auto plan=*defensive_ground_ward_plan(938,0);
 check(!defensive_ground_ward({1,25,0},80,2,0,0,0,plan,100));
 check(!defensive_ground_ward({1,25,1},0,2,0,0,0,plan,100));
 check(!defensive_ground_ward({1,25,1},80,0,0,0,0,plan,100));
 check(!defensive_ground_ward({1,25,1},80,2,65536,0,0,plan,100));
 check(!defensive_ground_ward({1,25,1},80,2,0,0,0,plan,UINT64_MAX-9999));
 const float x=std::bit_cast<float>(0xc5b6213cu),y=std::bit_cast<float>(0x44f3e791u);
 check(skills::particles::ward_stability_ground_2022(x,y,0)==bytes("a0803c21b6c591e7f34400000000000064010000"));
 check(combat::detail::integer(21,25,1187)==bytes("9e001500000019000000a3040000"));
 std::cout<<cases<<" rank ward duration/geometry/team/spirit/lifetime cases and exact frozen body1187/ground356 forms pass; World/Runtime consumers absent\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
