#include "gw_world_simulation.h"
#include <iostream>
using namespace fw::gw;
int main(){
 world::Instance w;
 w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
 w.join(2,"Selected",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 // An area recipient's clock is ahead of this request. Reject without
 // spending the selected recipient's stance or retaliating against caster.
 w.join(3,"Nearby",90,0,std::make_shared<world::Outbox>(),0,{}, {},300,2);
 for(unsigned id:{1u,2u,3u})w.set_vitals(id,{500,500,100,100},100);
 const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2);
 if(!w.hex_breaker(owner,*skills::effects::hex_breaker(12),20,100))return 2;
 const bool accepted=w.degeneration_hex(caster,owner,*skills::effects::degeneration_hex(108,12),200);
 const auto status=*w.status_snapshot(owner,200);
 const bool preserved=std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==10;});
 if(accepted||!preserved||w.resources(1)->health!=500){
  std::cerr<<"FAIL rejected area application mutated protection: accepted="<<accepted<<" stance="<<preserved<<" casterHealth="<<w.resources(1)->health<<'\n';return 1;
 }
 std::cout<<"PASS rejected area application preserves stance and caster Health\n";
}
