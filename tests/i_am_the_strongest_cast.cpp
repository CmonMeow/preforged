#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("I Am the Strongest cast line "+std::to_string(at.line()));
}
int main()try{
    for(bool hook_present:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto owner=*w.effect_scope(1);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        unsigned paid=0,completed=0;
        hooks.resources=[&](auto actor){return w.resources(actor);};
        hooks.events=[&](const auto& packets){box->enqueue(packets);};
        hooks.consume_energy_at=[&](auto cost,auto at){++paid;return cost==5&&w.consume_energy(1,cost,owner,at);};
        hooks.target_scope=[&](auto actor){return w.effect_scope(actor);};
        hooks.target_valid=[](auto actor,float,bool){return actor==1;};
        hooks.norn_rank=[](){return std::optional<unsigned>{0};};
        if(hook_present)hooks.i_am_the_strongest=[&](auto scope,const auto& plan,auto at){
            ++completed;return w.i_am_the_strongest(scope,plan,at);
        };
        skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2355]=1;
        skills::Slots bar{};bar[0]={2355,0};attributes::Ranks ranks{};
        box->drain();
        const auto result=runtime.start(100,{2355,0,0},bar,policy,ranks,true,hooks);
        if(result!=(hook_present?skills::casting::Reason::accepted:skills::casting::Reason::unsupported))
            std::cerr<<"hook="<<hook_present<<" reason="<<int(result)<<" paid="<<paid<<" completed="<<completed<<'\n';
        check(result==(hook_present?skills::casting::Reason::accepted:skills::casting::Reason::unsupported));
        check(paid==unsigned(hook_present));
        if(!hook_present){check(completed==0);continue;}
        runtime.update(100,hooks);
        check(completed==1&&w.resources(1)->energy==95);
        check(w.i_am_the_strongest_snapshot(owner,100)->remaining==5);
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),skills::casting::generic(48,1,2355))==1);
        check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,1909))==1);
        check(runtime.start(101,{2355,0,0},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
    }
    std::cout<<"PASS paid instant I Am the Strongest cast and World body\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
