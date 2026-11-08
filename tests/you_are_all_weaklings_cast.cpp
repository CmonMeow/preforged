#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Dwarf cast line "+std::to_string(at.line()));
}
int main()try{
    for(bool hook_present:{false,true})for(bool npc_target:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        const unsigned foe=npc_target?100:2;
        if(npc_target){
            world::Instance::Npc n;n.id=n.definition.id=foe;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=foe;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={1000,1000};d.damageable=true;d.conditions_allowed=true;
            d.knockdown_allowed=true;d.knockdown_evidence="test authored control";
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        if(!npc_target){
            w.join(foe,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(foe,{1000,1000,100,100},100);
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(foe);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        unsigned paid=0,completed=0;
        hooks.resources=[&](auto actor){return w.resources(actor);};
        hooks.events=[&](const auto& packets){w.skill_event(1,packets,100);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){++paid;return cost==5&&w.consume_energy(1,cost,source,at);};
        hooks.target_scope=[&](auto actor){return w.effect_scope(actor);};
        hooks.projectile_target_valid=[&](auto actor,float range){return actor==foe&&range==1248.f;};
        hooks.target_valid=[](auto,float,bool){return false;}; // No allied target fallback.
        hooks.norn_rank=[](){return std::optional<unsigned>{0};};
        if(hook_present)hooks.you_are_all_weaklings=[&](auto selected,const auto& plan,auto at){
            ++completed;std::function<void()> publication;
            const bool accepted=w.stage_you_are_all_weaklings(source,selected,plan,at,&publication);
            return skills::casting::EffectCompletion{accepted,{},std::nullopt,std::move(publication)};
        };
        skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2359]=1;
        skills::Slots bar{};bar[0]={2359,0};attributes::Ranks ranks{};
        box->drain();
        const auto result=runtime.start(100,{2359,0,foe},bar,policy,ranks,true,hooks);
        check(result==(hook_present?skills::casting::Reason::accepted:skills::casting::Reason::unsupported));
        check(paid==unsigned(hook_present));
        if(!hook_present){check(completed==0);continue;}
        runtime.update(100,hooks);
        check(completed==1&&w.resources(1)->energy==95);
        if(npc_target)check(w.npc_combat_snapshot(target)->health->current==1000);
        else check(w.resources(foe)->health==1000);
        const auto packets=box->drain();
        check(std::any_of(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==226&&p[1]==0;}));
        
        const auto caster_body=combat::detail::integer(21,1,1914);
        const auto target_body=skills::particles::targeted_body_2022(foe,1,2359);
        check(bool(target_body));
        const auto caster_packet=std::find(packets.begin(),packets.end(),caster_body);
        const auto target_packet=std::find(packets.begin(),packets.end(),*target_body);
        check(caster_packet!=packets.end()&&target_packet!=packets.end()&&caster_packet<target_packet);
        check(runtime.start(101,{2359,0,foe},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
        check(paid==1&&completed==1);
    }
    std::cout<<"PASS paid instant dwarf cast, scoped player/NPC World completion and packet publication\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
