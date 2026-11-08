#include "gw_world_simulation.h"
#include <iostream>
using namespace fw::gw;
int main(){
 world::Instance w;
 w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
 w.join(2,"Selected",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.join(3,"Nearby",90,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 for(unsigned id:{1u,2u,3u})w.set_vitals(id,{id==1?20u:500u,500,100,100},100);
 const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2),nearby=*w.effect_scope(3);
 if(!w.hex_breaker(owner,*skills::effects::hex_breaker(12),20,100))return 2;
 if(!w.degeneration_hex(caster,owner,*skills::effects::degeneration_hex(126,12),200))return 3;
 if(w.resources(1)->health!=0)return 4;
 // Existing World lifetime policy: a caster's death removes life-drain links.
 // A same-transaction application must not recreate them after death cleanup.
 const auto effects=*w.status_snapshot(nearby,200);
 if(std::any_of(effects.begin(),effects.end(),[](const auto& e){return e.skill==126;})){
  std::cerr<<"FAIL Life Transfer committed after caster death cleanup\n";return 1;
 }
 w.tick(1200);
 if(w.resources(1)->health!=0||w.resources(3)->health!=500)return 5;
 std::cout<<"PASS lethal retaliation does not restore life-drain state after death cleanup\n";
}
