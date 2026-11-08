#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
void join(world::Instance& w,unsigned id,unsigned team,float x=0){
 w.join(id,"Ally fixture",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,team);
 w.set_vitals(id,{500,1000,30,30},100);
}
int main()try{
 world::Instance w;join(w,1,1);join(w,2,1,50);join(w,3,2,60);join(w,4,1,1300);
 require(w.valid_heal_target(1,2,1248),"known allied team must not require party membership");
 require(!w.valid_heal_target(1,3,1248),"opposing team admitted");
 require(!w.valid_heal_target(1,4,1248),"range restriction lost");
 require(w.valid_heal_target(1,1,0)&&!w.valid_heal_target(1,1,0,false),"self/other-ally rule lost");
 const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
 const auto weapon=*skills::effects::weapon_spell(793,0,8,0);
 require(w.valid_weapon_spell_target(source,target,weapon,100),"weapon spell rejects nonparty ally");
 auto stale=target;++stale.generation;
 require(!w.valid_weapon_spell_target(source,stale,weapon,100),"atomic target validator accepted stale generation");
 require(w.heal_target(1,2,5,1248,true,100)&&w.resources(2)->health==505,"nonparty ally healing not committed");
 const auto party=*skills::effects::party_heal(287,0,0);
 require(w.party_heal(source,party,100),"party heal failed");
 require(w.resources(1)->health==500+party.base_heal&&w.resources(2)->health==505,"party-only effect leaked to nonparty ally");
 require(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100),"party setup failed");
 require(w.party_heal(source,party,100)&&w.resources(2)->health==505+party.base_heal,"actual party member omitted");
 world::Instance unknown;join(unknown,1,0);join(unknown,2,0,50);
 require(!unknown.valid_heal_target(1,2,1248),"unknown team equality created alliance");
 require(unknown.valid_heal_target(1,1,0),"self ally needs no inferred faction");
 require(unknown.party_action(1,158,2,100)&&unknown.party_action(2,154,1,100),"unknown-team party setup failed");
 require(unknown.valid_heal_target(1,2,1248),"authoritative party alliance ignored");
 std::cout<<"Ally scopes distinguish team alliance, party-only recipients and unknown relationships\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
