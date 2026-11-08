#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Aneurysm cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},1000,1);
        w.join(2,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},1000,2);
        w.set_vitals(1,{1000,1000,30,30},100);
        w.set_vitals(2,{1000,1000,10,30},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        unsigned paid=0,completed=0;
        hooks.events=[&](const auto& packets){box->enqueue(packets);};
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){++paid;check(cost==5);return w.consume_energy(1,cost,source,at);};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(source,id,range,1000);};
        if(mode!=1)hooks.aneurysm=[&](auto foe,const auto& plan,auto at)->skills::casting::EffectCompletion{
            ++completed;std::function<void()> publication;
            const bool accepted=w.stage_aneurysm(source,foe,plan,20,at,&publication);
            return {accepted,{}, {},std::move(publication)};
        };
        skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[2055]=1;
        skills::Slots bar{};bar[0]={2055,0};attributes::Ranks ranks{};ranks[2]=12;
        box->drain();
        const auto start=runtime.start(1000,{2055,0,2},bar,policy,ranks,true,hooks);
        check(start==(mode==1?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
        if(mode==1){check(paid==0&&completed==0);continue;}
        check(paid==1&&w.resources(1)->energy==25&&w.resources(2)->health==1000);
        if(mode==2)runtime.cancel(hooks);
        if(mode==3)check(w.kill(target,1500));
        runtime.update(1999,hooks);check(completed==0);
        runtime.update(2000,hooks);
        if(mode){check(completed==0&&w.resources(2)->health==(mode==3?0u:1000u));continue;}
        check(completed==1&&w.resources(2)->health==940&&w.resources(2)->energy==30);
        const auto packets=box->drain();
        const auto caster_body=*skills::casting::skill_success_body_effect(1,2055);
        const auto target_body=*skills::particles::targeted_body_2022(2,1,2055);
        const auto caster_it=std::find(packets.begin(),packets.end(),caster_body);
        const auto target_it=std::find(packets.begin(),packets.end(),target_body);
        check(caster_it!=packets.end()&&target_it!=packets.end()&&caster_it<target_it);
        check(std::count(packets.begin(),packets.end(),caster_body)==1);
        check(runtime.start(2001,{2055,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::busy);
    }
    std::cout<<"PASS Aneurysm opt-in cast payment, interruption, scoped completion and body ordering\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
