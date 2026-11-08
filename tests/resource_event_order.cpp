#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Resource event line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<4;++mode){world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,50,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.wound_spell(source,target,*skills::effects::wound_spell(44,0),20,100));check(w.resources(2)->max_health==500);
  const bool ok=mode==0?w.consume_energy(2,5,target,10100):mode==1?w.restore_energy(2,5,target,10100):mode==2?w.energy_capacity(2,5,0,10100):w.settle_resources(target,10100);
  check(ok&&w.resources(2)->max_health==400&&w.has_condition(target,combat::conditions::Kind::deep_wound,10100));
  check(!w.consume_energy(2,0,target,10099)&&!w.restore_energy(2,1,target,10099)&&!w.energy_capacity(2,5,0,10099));
 }
 world::Instance w;w.join(1,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,50,100},100);const auto owner=*w.effect_scope(1);
 check(w.energy_capacity(1,5,0,100,{80,0}));check(w.consume_energy(1,2,owner,1600)&&w.resources(1)->energy==49);
 check(w.consume_energy(1,3,owner,1600)&&w.resources(1)->energy==46);check(!w.consume_energy(1,1,owner,1599)&&w.resources(1)->energy==46);
 check(w.restore_energy(1,5,owner,3100)&&w.resources(1)->energy==52);
 check(w.energy_capacity(1,5,0,3100));check(w.restore_energy(1,30,owner,3100)&&w.resources(1)->energy==2);
 auto stale=owner;++stale.generation;check(!w.settle_resources(stale,5000)&&!w.consume_energy(1,0,stale,5000));check(w.settle_resources(owner,3100));
 std::cout<<"PASS timestamped costs/returns/capacity settle real enchantment end effects first, integrate Energy once and reject stale clocks/owners\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
