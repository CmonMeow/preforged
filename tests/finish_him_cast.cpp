#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Finish Him cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned health:{500u,499u})for(bool hook_present:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.set_vitals(2,{health,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        unsigned paid=0,completed=0;
        std::string phase;
        hooks.diagnostic=[&](auto,const char* value){phase=value;};
        hooks.resources=[&](auto actor){return w.resources(actor);};
        hooks.events=[&](const auto& packets){w.skill_event(1,packets,100);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){++paid;check(cost==10);return w.consume_energy(1,cost,source,at);};
        hooks.target_scope=[&](auto actor){return w.effect_scope(actor);};
        hooks.projectile_target_valid=[&](auto actor,float range){return actor==2&&range==1248.f;};
        hooks.target_valid=[](auto,float,bool){return false;}; // Foe completion must not use allied-heal validation.
        hooks.norn_rank=[](){return std::optional<unsigned>{0};};
        if(hook_present)hooks.finish_him=[&](auto foe,const auto& plan,auto at){
            ++completed;std::function<void()> publication;
            const bool accepted=w.stage_finish_him(source,foe,plan,20,at,&publication);
            return skills::casting::EffectCompletion{accepted,{},std::nullopt,std::move(publication)};
        };
        skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2353]=1;
        skills::Slots bar{};bar[0]={2353,0};attributes::Ranks ranks{};
        box->drain();
        const auto result=runtime.start(100,{2353,0,2},bar,policy,ranks,true,hooks);
        check(result==(hook_present?skills::casting::Reason::accepted:skills::casting::Reason::unsupported));
        check(paid==unsigned(hook_present));
        if(!hook_present){check(completed==0&&w.resources(2)->health==health);continue;}
        runtime.update(100,hooks);
        if(completed!=1||w.resources(1)->energy!=90)throw std::runtime_error(
            "completion="+std::to_string(completed)+" energy="+std::to_string(w.resources(1)->energy)+" phase="+phase);
        if(w.resources(2)->health!=(health==500?500:355))throw std::runtime_error(
            "target="+std::to_string(w.resources(2)->health)+" initial="+std::to_string(health)+" phase="+phase);
        const auto packets=box->drain();
        check(std::any_of(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>1&&packet[0]==226&&packet[1]==0;}));
        if(health==499)check(std::count_if(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>2&&packet[0]==65&&packet[1]==0;})==2);
    }
    {
        world::Instance w;world::Instance::Npc n;
        n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health={499,1000};d.damageable=true;d.conditions_allowed=true;
        n.combat_definition=d;w.configure_npcs({n});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        hooks.resources=[&](auto actor){return w.resources(actor);};
        hooks.events=[&](const auto& packets){w.skill_event(1,packets,100);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){return cost==10&&w.consume_energy(1,cost,source,at);};
        hooks.target_scope=[&](auto actor){return w.effect_scope(actor);};
        hooks.projectile_target_valid=[&](auto actor,float range){return actor==100&&range==1248.f;};
        hooks.target_valid=[](auto,float,bool){return false;};
        hooks.norn_rank=[](){return std::optional<unsigned>{0};};
        hooks.finish_him=[&](auto foe,const auto& plan,auto at){
            std::function<void()> publication;
            const bool accepted=w.stage_finish_him(source,foe,plan,20,at,&publication);
            return skills::casting::EffectCompletion{accepted,{},std::nullopt,std::move(publication)};
        };
        skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2353]=1;
        skills::Slots bar{};bar[0]={2353,0};attributes::Ranks ranks{};
        box->drain();
        check(runtime.start(100,{2353,0,100},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
        const auto post=w.npc_combat_snapshot(target);
        check(post&&post->health->current==355&&post->health->maximum==900&&w.resources(1)->energy==90);
    }
    std::cout<<"PASS opt-in Finish Him paid instant cast, Norn rank and trusted World completion\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
