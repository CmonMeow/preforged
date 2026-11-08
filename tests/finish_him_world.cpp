#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace fw::gw::skills::effects;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Finish Him World line "+std::to_string(at.line()));
}
int main()try{
    for(bool reversal:{false,true}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*40),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?1000u:499u,1000,100,100},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.status_mutation(target,1399,80,12,100,6100,true));
        if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
        check(w.stage_finish_him(source,target,*finish_him(0),20,100));
        const auto before=w.resources(2)->health;
        check(w.stage_finish_him(source,target,*finish_him(0),20,101));
        check(w.resources(2)->health==before-34);
    }

    for(bool reversal:{false,true}){
        world::Instance w;world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=100;npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{499,1000};d.damageable=d.conditions_allowed=true;
        d.healing_allowed=true;d.healing_evidence="authored conversion fixture";npc.combat_definition=d;w.configure_npcs({npc});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        check(w.stage_npc_shield_absorption(target,12,100));if(reversal)check(w.stage_npc_reversal(target,12,100));
        check(w.stage_finish_him(source,target,*finish_him(0),20,100));
        const auto before=w.npc_combat_snapshot(target)->health->current;
        check(w.stage_finish_him(source,target,*finish_him(0),20,101));
        check(std::abs(w.npc_combat_snapshot(target)->health->current-(before-34))<1e-7);
    }
    for(unsigned health:{500u,499u,120u,100u}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u}){
            w.join(id,"Actor",float(id*40),0,id==1?box:std::make_shared<world::Outbox>(),0,
                   {},{},100,id==1?1:2);
            w.set_vitals(id,{id==1?1000u:health,1000,100,100},100);
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        std::function<void()> publish;box->drain();
        check(w.stage_finish_him(source,target,*finish_him(0),20,100,&publish));
        check(bool(publish));
        if(health==500){check(w.resources(2)->health==500);check(w.status_snapshot(target,100)->empty());}
        if(health==499)check(w.resources(2)->health<499);
        if(health==120)check(w.resources(2)->health==0); // Deep Wound first makes the 44-damage hit lethal.
        if(health==100)check(w.resources(2)->health<100);
        check(box->drain().empty());publish();
        const auto packets=box->drain();
        check(packets.empty()==(health==500));
        if(health==499)check(std::count_if(packets.begin(),packets.end(),[](const auto& packet){return packet.size()>2&&packet[0]==65&&packet[1]==0;})==2);
        auto stale=target;++stale.generation;
        check(!w.stage_finish_him(source,stale,*finish_him(0),20,101));
    }
    for(bool conditions_allowed:{true,false})for(unsigned health:{499u,120u}){
        world::Instance w;world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=100;npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;definition.binding.map=146;
        definition.binding.actor=definition.binding.definition=100;
        definition.relation=combat::npcs::Relation::hostile_to_players;
        definition.spawn_health=combat::Health{double(health),1000};
        definition.damageable=true;definition.conditions_allowed=conditions_allowed;
        definition.earth_armor=definition.fire_armor=definition.cold_armor=
            definition.lightning_armor=definition.physical_armor=60;
        npc.combat_definition=definition;w.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);
        std::function<void()> publish;box->drain();
        check(w.stage_finish_him(owner,target,*finish_him(0),20,100,&publish));
        const auto post=w.npc_combat_snapshot(target);
        check(post&&post->health->current<health&&box->drain().empty());
        check(post->health->maximum==(conditions_allowed?900:1000)); // Deep Wound reduction caps at 100 Health.
        if(health==120)check(post->health->current==(conditions_allowed?0:76));
        publish();check(!box->drain().empty());
    }
    {
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.join(2,"Bonus",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.set_vitals(2,{450,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.stage_endure_pain_health(target,*endure_pain(0),100));
        check(w.resources(2)->max_health==1090&&w.resources(2)->health==540);
        box->drain();
        check(w.stage_finish_him(source,target,*finish_him(0),20,100));
        check(w.resources(2)->health==540&&box->drain().empty());
    }
    std::cout<<"PASS Finish Him World player/NPC threshold, conditions, deferred publication and scope guard\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
