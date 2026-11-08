#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Armor Earth World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<5;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Earth",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1);
  check(w.armor_earth(owner,0,100));auto status=*w.status_snapshot(owner,100);check(status.size()==1&&status[0].skill==165&&status[0].expires==30100);auto effect=status[0].instance;
  world::Packet body;world::u16(body,158);world::u32(body,21);world::u32(body,1);world::u32(body,297);auto packets=box->drain();check(std::count(packets.begin(),packets.end(),body)==1);
  check(!w.status_mutation(owner,165,999,12,100,30100,true));
  w.move(1,10000,0,100);w.tick(600);check(std::abs(w.position(1,600).first-72)<.3);
  if(mode==1){check(w.status_mutation(owner,165,effect,0,600,{},false));w.tick(1100);check(std::abs(w.position(1,1100).first-216)<.3);}
  if(mode==2){check(w.armor_earth(owner,12,600));w.tick(1100);check(std::abs(w.position(1,1100).first-(72+113.76))<.3);check(w.status_snapshot(owner,1100)->size()==1);}
  if(mode==3){check(w.kill(owner,600));check(w.status_snapshot(owner,600)->empty());}
  if(mode==4){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},600,1);w.set_vitals(1,{1000,1000,100,100},100);check(!w.armor_earth(owner,12,600));const auto fresh=*w.effect_scope(1);check(w.status_snapshot(fresh,600)->empty());w.move(1,10000,0,600);w.tick(1100);check(std::abs(w.position(1,1100).first-144)<.3);}
  if(mode==0){w.tick(30100);check(std::abs(w.position(1,30100).first-4320)<.5&&w.status_snapshot(owner,30100)->empty());w.tick(31100);check(std::abs(w.position(1,31100).first-4608)<.5);}
 }
 MovementEffects m;m.apply({1,1,165,12,Category::enchantment,100,30100});m.apply({2,1,1043,0,Category::stance,100,3100});check(std::abs(m.at(100)-1.185f)<1e-5);m.apply({3,2,76,0,Category::hex,100,1100});check(std::abs(m.at(100)-.5925f)<1e-5);m.remove(3,600);check(std::abs(m.integral(100,1100)-888.75)<.01);
 std::cout<<"PASS Armor Earth World owned timer/body297, renewal/removal/expiry/death/rejoin and real movement plus multiplicative overlap\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
