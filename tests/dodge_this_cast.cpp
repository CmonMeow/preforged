#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Dodge This cast line "+std::to_string(at.line()));
}
int main()try{
    for(bool hook_present:{false,true})for(bool charged:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto owner=*w.effect_scope(1);
        skills::Slots bar{};bar[0]={2354,0};
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        unsigned paid=0,completed=0;
        hooks.resources=[&](auto actor){return w.resources(actor);};
        hooks.events=[&](const auto& packets){box->enqueue(packets);};
        hooks.consume_energy_at=[&](auto cost,auto at){return cost==0&&w.consume_energy(1,cost,owner,at);};
        hooks.target_scope=[&](auto actor){return w.effect_scope(actor);};
        hooks.target_valid=[](auto actor,float,bool){return actor==1;};
        hooks.norn_rank=[](){return std::optional<unsigned>{0};};
        hooks.consume_adrenaline=[&](auto slot,auto){++paid;return charged&&slot.skill==2354&&slot.copy==0;};
        hooks.reconcile_adrenaline=[](auto){};
        if(hook_present)hooks.dodge_this=[&](auto scope,const auto& plan,auto at){
            ++completed;return w.dodge_this(scope,plan,at);
        };
        skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2354]=1;
        attributes::Ranks ranks{};box->drain();
        const auto result=runtime.start(100,{2354,0,0},bar,policy,ranks,true,hooks);
        const auto expected=!hook_present?skills::casting::Reason::unsupported:
            !charged?skills::casting::Reason::adrenaline:skills::casting::Reason::accepted;
        if(result!=expected)std::cerr<<"hook="<<hook_present<<" charged="<<charged<<" result="<<int(result)<<" expected="<<int(expected)<<"\n";
        check(result==expected);
        check(paid==unsigned(hook_present));
        if(expected!=skills::casting::Reason::accepted){check(completed==0);continue;}
        runtime.update(100,hooks);
        check(completed==1&&w.dodge_this_armed(owner,100));
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),skills::casting::generic(48,1,2354))==1);
    }
    std::cout<<"PASS Dodge This paid instant cast and World effect\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
