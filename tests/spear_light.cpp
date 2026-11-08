#include "gw_spear_light.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <limits>
using namespace fw::gw;using namespace skills::effects;
void check(bool value){if(!value)throw std::runtime_error("Spear of Light policy");}
int main()try{
 unsigned contexts=0;
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=spear_light(rank);check(p&&p->damage==26+2*rank&&p->attacking_bonus==15+3*rank);
  for(bool light:{false,true})for(bool attacking:{false,true}){
   HolyTraits traits;traits.light_sensitive=light;
   const auto hits=spear_light_hits(*p,traits,20,attacking);check(hits&&hits->size()==(attacking?2u:1u));
   check(hits->at(0).amount==(26+2*rank)*(light?2:1));
   check(hits->at(0).kind==combat::Kind::armor_ignoring&&hits->at(0).skill==1130);
   if(attacking)check(hits->at(1).amount==(15+3*rank)*(light?2:1));
   ++contexts;
  }
  check(!spear_light_hits(*p,{},20,false));
  HolyTraits traits;traits.light_sensitive=false;
  check(!spear_light_hits(*p,traits,20,std::nullopt));
  auto bad=*p;++bad.damage;check(!spear_light_hits(bad,traits,20,true));
  bad=*p;++bad.attacking_bonus;check(!spear_light_hits(bad,traits,20,true));
 }

 for(unsigned mode=0;mode<13;++mode){
  SpearLightActor source{{1,42,1},0,0,0,true};SpearLightTarget target;
  static_cast<SpearLightActor&>(target)={{1,43,1},900,0,0,true};target.hostile=true;target.traits.light_sensitive=false;target.attacking=false;
  SpearLightFlight flight;check(flight.launch(*spear_light(12),source,target,{1800,32},0,0,1000));
  check(flight.arrives()==1500&&flight.aim().x==900);
  check(!flight.impact(source,target,20,1499)&&flight.pending());
  switch(mode){
   case 0:target.attacking=true;break; // Fact changes after release.
   case 1:target.x=933;break;
   case 2:target.plane=1;break;
   case 3:target.scope.generation=2;break;
   case 4:source.scope.generation=2;break;
   case 5:target.alive=false;break;
   case 6:source.alive=false;break;
   case 7:target.hostile=false;break;
   case 8:target.attacking.reset();break;
   case 9:target.traits.light_sensitive.reset();break;
   case 10:target.x=std::numeric_limits<float>::quiet_NaN();break;
   case 11:check(!flight.impact(source,target,20,1498)&&!flight.pending());continue;
   case 12:break;
  }
  const auto hit=flight.impact(source,target,20,1500);
  if(mode==0||mode==12)check(hit&&hit->hits.size()==(mode==0?2u:1u)&&hit->at==1500);
  else check(!hit);
  check(!flight.pending()&&!flight.impact(source,target,20,1500));
 }
 {
  const auto packet=skills::particles::spear_light_launch_2022(42,900,0,2,.5f,7);
  check(packet.size()==29&&packet[0]==163&&packet[1]==128&&packet.back()==0);
  const auto word=[&](unsigned at){return unsigned(packet[at])|(unsigned(packet[at+1])<<8)|(unsigned(packet[at+2])<<16)|(unsigned(packet[at+3])<<24);};
  check(word(20)==1266&&word(24)==7&&packet[14]==2);
  bool rejected=false;try{(void)skills::particles::spear_light_launch_2022(42,0,0,0,1,0);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 {
  SpearLightActor source{{1,42,1},0,0,0,true};SpearLightTarget target;
  static_cast<SpearLightActor&>(target)={{1,43,1},900,0,0,true};target.hostile=true;target.traits.light_sensitive=false;target.attacking=false;
  SpearLightFlight f;check(f.launch(*spear_light(0),source,target,{1800,32},0,0,1000));
  check(f.publish_at(1200)&&f.arrives()==1700);
  check(!f.impact(source,target,20,1699)&&f.pending());check(bool(f.impact(source,target,20,1700)));
 }
 check(!spear_light(21));
 std::cout<<"PASS Spear of Light "<<contexts<<" impact-plan contexts; no delivery binding claimed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
