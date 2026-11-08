#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>

using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Iron Palm cast line "+std::to_string(at.line()));
}

int main()try{
    for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={1000,1000};d.damageable=true;d.evidence.armor="test";
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        if(!npc){
            w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{1000,1000,100,100},100);
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        unsigned paid=0,completed=0;bool stale=false;
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.events=[&](const auto& packets){w.skill_event(1,packets,1750);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){++paid;check(cost==5);return w.consume_energy(1,cost,source,at);};
        hooks.target_scope=[&](auto id){return stale&&id==target.actor?std::optional<skills::effects::Scope>{}:w.effect_scope(id);};
        hooks.projectile_target_valid=[&](auto id,float range){return id==target.actor&&range==144.f;};
        if(mode!=1)hooks.iron_palm=[&](auto foe,const auto& plan,auto at)->skills::casting::EffectCompletion{
            ++completed;std::function<void()> publication;
            const bool accepted=w.iron_palm(source,foe,plan,20,at,nullptr,&publication);
            return {accepted,{},std::nullopt,std::move(publication)};
        };
        skills::Policy policy;policy.primary=7;policy.campaigns=255;policy.learned[786]=1;
        skills::Slots bar{};bar[0]={786,0};attributes::Ranks ranks{};ranks[30]=15;
        box->drain();
        const auto result=runtime.start(1000,{786,0,target.actor},bar,policy,ranks,true,hooks);
        check(result==(mode==1?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
        if(mode==1){check(paid==0&&completed==0);continue;}
        check(paid==1&&w.resources(1)->energy==95);
        if(mode==2)runtime.cancel(hooks);
        if(mode==3)runtime.interrupt(1100,hooks);
        if(mode==4)stale=true;
        runtime.update(1749,hooks);check(completed==0);
        runtime.update(1750,hooks);
        check(completed==unsigned(mode==0));
        const auto packets=box->drain();
        const auto body=skills::casting::success_body_effect(1,786);
        const auto finish=std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1));
        const auto shown=std::find(packets.begin(),packets.end(),body);
        const auto chain=skills::effects::iron_palm_chain_2022(source.actor,target.actor,1);
        const auto chain_at=std::find(packets.begin(),packets.end(),chain);
        check((shown!=packets.end())==(mode==0));
        check((chain_at!=packets.end())==(mode==0));
        if(mode==0){
            check(finish!=packets.end()&&finish<shown&&shown<chain_at);
            check(std::count(packets.begin(),packets.end(),chain)==1);
            const auto damage=std::find_if(shown,chain_at,[](const auto& packet){
                return packet.size()>=6&&packet[0]==162&&packet[1]==0&&
                    packet[2]==55&&packet[3]==0&&packet[4]==0&&packet[5]==0;
            });
            check(damage!=chain_at&&shown<damage&&damage<chain_at);
        }
        const auto health=npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);
        check(health==(mode==0?950:1000));
        check(w.dagger_stage(source,target,1750)==unsigned(mode==0));
    }
    std::cout<<"PASS staged Iron Palm paid cast, touch range, player/NPC hit, cancellation, interruption and stale target\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
