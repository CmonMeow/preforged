#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool ok){if(!ok)throw std::runtime_error("Recall/Channeling order");}
int main()try{
 for(bool success:{false,true}){world::Instance w;
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,50,100},100);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(1);
  check(w.energy_capacity(1,5,0,100,{80,-2}));check(w.status_mutation(owner,38,10,12,100,46100,true));check(w.mantra_recall(owner,12,100));
  check(w.prepare_channeling(owner,281,1,20099));check(w.complete_channeling(owner,1,success,20100));
  check(w.resources(1)->energy==(success?73u:50u));check(!w.complete_channeling(owner,1,true,20100));
  if(!success){check(w.settle_resources(owner,20100));check(w.resources(1)->energy==72);}
  check(w.consume_energy(1,success?73u:72u,owner,20100));check(w.resources(1)->energy==0);
  check(w.settle_resources(owner,20200)&&w.resources(1)->energy==0);
 }
 std::cout<<"PASS Recall expires before successful Channeling return; failed cleanup preserves its end event, subsequent cost sees each reward once\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
