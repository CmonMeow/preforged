#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Aneurysm World line "+std::to_string(at.line()));
}
static world::Instance::Npc hostile(unsigned id,float x,
        std::optional<combat::npcs::Energy> energy={combat::npcs::Energy{10,30}}){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.x=x;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;
    d.binding.actor=d.binding.definition=id;d.binding.level=n.definition.level;
    d.relation=combat::npcs::Relation::hostile_to_players;
    d.spawn_health=combat::Health{1000,1000};d.damageable=true;
    d.spawn_energy=energy;if(energy)d.energy_pips=0;n.combat_definition=d;return n;
}
int main()try{
    const auto plan=*skills::effects::aneurysm(12);
    for(bool reversal:{false,true}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,10,30},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.status_mutation(target,1399,80,12,100,6100,true));
        if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
        check(w.stage_aneurysm(source,target,plan,20,100));check(w.resources(2)->health==(reversal?1000u:945u));
        const auto before=w.resources(2)->health;w.set_vitals(2,{before,1000,10,30},100);
        check(w.stage_aneurysm(source,target,plan,20,101));check(w.resources(2)->health==before-50);
    }

    {
        world::Instance w;
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.join(2,"Primary",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.join(3,"Adjacent",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.join(4,"Ally",115,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(5,"Distant",400,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,30,30},100);
        w.set_vitals(2,{1000,1000,10,30},100);
        w.set_vitals(3,{1000,1000,7,30},100);
        w.set_vitals(4,{1000,1000,5,30},100);
        w.set_vitals(5,{1000,1000,8,30},100);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
        box->drain();
        std::function<void()> publish;
        check(w.stage_aneurysm(caster,target,plan,20,100,&publish));
        check(bool(publish)&&box->drain().empty());
        check(w.resources(2)->health==940&&w.resources(2)->energy==30);
        check(w.resources(3)->energy==0&&w.resources(4)->energy==5&&
              w.resources(5)->energy==8);
        publish();check(!box->drain().empty());
        auto stale=target;++stale.generation;
        check(!w.stage_aneurysm(caster,stale,plan,20,100));
    }
    {
        world::Instance w;
        w.configure_npcs({hostile(100,100),hostile(101,120,{combat::npcs::Energy{7,30}})});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Adjacent player",115,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,30,30},100);
        w.set_vitals(2,{1000,1000,6,30},100);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(100),near=*w.effect_scope(101);
        check(w.stage_aneurysm(caster,target,plan,20,100));
        check(w.npc_combat_snapshot(target)->health->current==940);
        check(w.npc_energy_snapshot(target)->current==30);
        check(w.npc_energy_snapshot(near)->current==0);
        check(w.resources(2)->energy==0);
    }
    {
        world::Instance w;
        w.configure_npcs({hostile(100,100),hostile(101,120,std::nullopt)});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,30,30},100);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(100);
        check(!w.stage_aneurysm(caster,target,plan,20,100));
        check(w.npc_combat_snapshot(target)->health->current==1000);
        check(w.npc_energy_snapshot(target)->current==10);
    }
    {
        // Overcast limits Energy actually received, not the amount generated
        // for Aneurysm damage and adjacent Energy loss.
        world::Instance w;
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Overcast foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.join(3,"Nearby foe",115,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,30,30},100);
        w.set_vitals(2,{1000,1000,10,30},100);
        w.set_vitals(3,{1000,1000,30,30},100);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.activate_overcast_energy(target,231,0,1,100));
        check(w.stage_aneurysm(caster,target,plan,20,100));
        check(w.resources(2)->energy==20&&w.resources(2)->health==940);
        check(w.resources(3)->energy==10);
    }
    std::cout<<"PASS Aneurysm World player/NPC primary, adjacent Energy, deferred publication and unknown authority\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
