#include "gw_damage_fields.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Snow Storm policy line "+std::to_string(at.line()));
}
combat::Packet bytes(std::string_view text){combat::Packet r;for(std::size_t n=0;n<text.size();n+=2)r.push_back(std::uint8_t(std::stoul(std::string(text.substr(n,2)),nullptr,16)));return r;}
int main()try{unsigned cases=0;
 for(unsigned rank=0;rank<=10;++rank)for(bool dense:{false,true}){
  const auto p=snow_storm(rank);check(p&&p->damage==30+2*std::min(rank,5u)&&p->pulses==5);
  const auto field=damage_field(2222,rank);check(field&&field->kind==FieldDamage::cold&&field->radius==166&&field->damage==p->damage);
  DamageFieldSchedule q;check(q.add({1,25,1},20,100,200,3,*field,100));unsigned pulses=0;
  const auto apply=[&](const DamageFieldPulse& hit){++pulses;check(hit.caster==Scope{1,25,1}&&hit.x==100&&hit.y==200&&hit.plane==3);
   check(hit.at==100+pulses*1000&&hit.plan==*field);return true;};
  if(dense)for(unsigned at=100;at<5100;at+=100)q.advance(at,apply);
  q.advance(5100,apply);q.advance(6100,apply);check(pulses==5&&q.empty());++cases;
 }
 check(!snow_storm(11)&&!damage_field(2222,11)&&!damage_field(2222,0,true));
 auto forged=*damage_field(2222,0);++forged.damage;DamageFieldSchedule bad;check(!bad.add({1,25,1},20,0,0,0,forged,100));
 check(!bad.add({0,25,1},20,0,0,0,*damage_field(2222,0),100));
 check(!bad.add({1,25,1},20,0,0,0,*damage_field(2222,0),UINT64_MAX-4999));
 const auto visuals=skills::particles::snow_storm_ground_2022(25,-5768,1978,0);
 check(visuals.size()==2&&visuals[0]==bytes("a0800040b4c50040f744000019000000a4070000"));
 check(visuals[1]==bytes("a0800040b4c50040f7440000000000007d010000"));
 bool rejected=false;try{skills::particles::snow_storm_ground_2022(0,0,0,0);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 std::cout<<cases<<" all-title dense/sparse fixed-centre cold-field schedule cases and two exact frozen positional packet forms pass; World/Runtime not bound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
