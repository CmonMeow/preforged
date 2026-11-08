#include "gw_world_simulation.h"
#include "gw_selfless_spirit_registry.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Selfless World line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned id:{1952u,2095u})for(unsigned rank:{0u,6u,12u}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
        w.set_vitals(1,{500,1000,100,100},100);const auto owner=*w.effect_scope(1);
        const auto plan=*selfless_spirit(id,rank);box->drain();
        check(w.complete_selfless_spirit(owner,plan,12,1000));
        auto state=w.selfless_spirit_state(owner,1000);check(state&&*state);
        check((**state).skill==id&&(**state).rank==rank&&(**state).expires==1000+plan.duration_ms);
        check(w.resources(1)->health==538);const auto first=(**state).instance;
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),combat::detail::integer(58,1,0))==1);
        check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,553))==1);
        const auto replacement=*selfless_spirit(id==1952?2095:1952,rank);
        check(w.complete_selfless_spirit(owner,replacement,0,1001));
        state=w.selfless_spirit_state(owner,1001);check(state&&*state&&(**state).instance!=first);
        check(w.status_snapshot(owner,1001)->size()==1);
        const auto expiry=*(**state).expires;const auto second=(**state).instance;
        check(w.selfless_spirit_state(owner,expiry-1).value().has_value());
        check(!w.selfless_spirit_state(owner,expiry).value().has_value());
        box->drain();w.tick(expiry+1);const auto again=box->drain();
        combat::Packet removed{67,0};combat::detail::put(removed,1);combat::detail::put(removed,second);
        check(std::count(again.begin(),again.end(),removed)==0);
        auto stale=owner;++stale.generation;check(!w.selfless_spirit_state(stale,expiry+1));
        check(!w.complete_selfless_spirit(stale,plan,0,expiry+1));
        auto forged=plan;++forged.reduction;check(!w.complete_selfless_spirit(owner,forged,0,expiry+1));
    }
    for(bool area:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u}){
            w.join(id,"Actor",float(id*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{500,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));
        }
        const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);
        check(w.complete_selfless_spirit(owner,*selfless_spirit(1952),0,1000));
        const auto entry=**w.selfless_spirit_state(owner,1000);box->drain();
        if(area)check(w.complete_chilblains(foe,owner,*chilblains(12),20,1001));
        else check(w.enchantment_removal(foe,owner,*enchantment_removal(69,12),20,1001));
        check(!w.selfless_spirit_state(owner,1001).value().has_value());
        combat::Packet removed{67,0};combat::detail::put(removed,1);combat::detail::put(removed,entry.instance);
        const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),removed)==1);
        w.tick(*entry.expires);const auto expired=box->drain();check(std::count(expired.begin(),expired.end(),removed)==0);
    }
    std::cout<<"Selfless World application/alias/Divine Favor/expiry/strip/stale scope passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
