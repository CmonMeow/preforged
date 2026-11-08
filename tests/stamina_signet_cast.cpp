#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
using namespace effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Stamina cast line "+std::to_string(at.line()));
}

int main()try{
    for(unsigned mode=0;mode<4;++mode){
        world::Instance world;auto box=std::make_shared<world::Outbox>();
        world.join(1,"Warrior",0,0,box,0,{}, {},100,1);
        world.set_vitals(1,{400,500,100,100},100);
        const auto owner=*world.effect_scope(1);
        casting::Runtime runtime(1);casting::Hooks hooks;unsigned completions=0;bool stale=false;
        hooks.resources=[&](auto id){return world.resources(id);};
        hooks.events=[&](const auto& packet){box->enqueue(packet);};
        hooks.consume_energy_at=[&](auto cost,auto now){return cost==0&&world.consume_energy(1,cost,owner,now);};
        hooks.target_scope=[&](auto id){auto scope=world.effect_scope(id);if(stale&&scope)++scope->generation;return scope;};
        hooks.target_valid=[](auto id,float,bool){return id==1;};
        if(mode!=1)hooks.stamina_signet=[&](auto source,const auto& plan,auto now){
            ++completions;std::function<void()> publication;
            const bool accepted=world.stage_stamina_signet_health(source,plan,now,&publication);
            return casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        };
        Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[1411]=1;
        Slots bar{};bar[0]={1411,0};attributes::Ranks ranks{};ranks[17]=mode==3?21:12;
        box->drain();const auto start=runtime.start(100,{1411,0,0},bar,policy,ranks,true,hooks);
        if(mode==1||mode==3){check(start==casting::Reason::unsupported&&completions==0);continue;}
        if(mode==2)stale=true;
        check(start==casting::Reason::accepted&&runtime.active());
        check(world.resources(1)->max_health==500);
        runtime.update(350,hooks);
        if(mode==2){check(completions==0&&world.resources(1)->max_health==500);continue;}
        check(!runtime.active()&&completions==1&&world.resources(1)->max_health==500+stamina_signet(12)->health_bonus);
        const auto wire=box->drain();
        const auto finish=std::find(wire.begin(),wire.end(),casting::terminal(58,1));
        const auto body=std::find(wire.begin(),wire.end(),*casting::skill_success_body_effect(1,1411));
        const auto status=std::find_if(wire.begin(),wire.end(),[](const auto& p){return p.size()>1&&p[0]==64&&p[1]==0;});
        check(finish!=wire.end()&&body!=wire.end()&&status!=wire.end()&&finish<body&&body<status);
        check(runtime.start(2000,{1411,0,0},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    }
    std::cout<<"PASS Signet of Stamina normal casting, target scope, ranked Health and finish/body/status order\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
