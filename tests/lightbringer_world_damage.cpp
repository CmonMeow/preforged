#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Lightbringer World damage line "+
                                     std::to_string(at.line()));
}
int main()try{
    world::Instance w;
    std::vector<world::Instance::Npc> npcs;
    for(unsigned id:{100u,101u,102u}){
        world::Instance::Npc npc;
        npc.id=npc.definition.id=id;
        npc.x=id==100?60.f:id==101?140.f:600.f;
        npc.definition.level=20;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;
        body.binding.map=474;
        body.binding.actor=body.binding.definition=id;
        body.binding.level=20;
        body.spawn_health={200,200};
        body.general_armor=id==101?100:60;
        body.general_armor_evidence="synthetic reviewed neutral armor";
        body.damageable=true;
        body.relation=combat::npcs::Relation::hostile_to_players;
        if(id==101){body.servant_of_abaddon=false;
            body.servant_of_abaddon_evidence="synthetic exception";}
        npc.combat_definition=body;
        npcs.push_back(npc);
    }
    w.configure_npcs(npcs);
    auto box=std::make_shared<world::Outbox>();
    w.join(1,"Lightbringer",0,0,box,0,{}, {},100,1);
    w.join(2,"Opposing player",220,0,std::make_shared<world::Outbox>(),
        0,{}, {},100,2);
    w.join(3,"Party ally",180,0,std::make_shared<world::Outbox>(),
        0,{}, {},100,1);
    w.set_vitals(1,{100,100,50,50},100);
    w.set_vitals(2,{200,200,50,50},100);
    w.set_vitals(3,{200,200,50,50},100);
    const auto source=*w.effect_scope(1);
    const auto enemy=*w.effect_scope(2);
    const auto primary=*w.effect_scope(100);
    const auto near=*w.effect_scope(101);
    const auto far=*w.effect_scope(102);
    const auto primary_action=w.begin_npc_action(primary,
        combat::npcs::control::Action::spell,900,3000,skills::Slot{194,0});
    const auto near_action=w.begin_npc_action(near,
        combat::npcs::control::Action::spell,900,3000,skills::Slot{194,0});
    check(primary_action&&near_action);
    box->drain();
    check(!w.stage_lightbringers_gaze_world(source,primary,
        *skills::effects::lightbringers_gaze(2),20,1000));
    check(w.npc_combat_snapshot(primary)->health->current==200);
    check(w.npc_combat_snapshot(near)->health->current==200);
    check(w.resources(2)->health==200);
    check(box->drain().empty());
    check(w.holy_armor_locations(enemy,std::array<int,5>{80,60,60,60,60}));
    unsigned armor_draws=0;
    w.configure_armor_location_draws([&]{++armor_draws;return 0u;});
    std::function<void()> publish;
    const auto selected=w.stage_lightbringers_gaze_world(source,primary,
        *skills::effects::lightbringers_gaze(2),20,1000,&publish);
    check(selected&&*selected==
        (std::vector<skills::effects::Scope>{primary,near,enemy}));
    check(w.npc_combat_snapshot(primary)->health->current==100);
    check(w.npc_combat_snapshot(near)->health->current==150);
    check(w.npc_combat_snapshot(far)->health->current==200);
    check(w.resources(2)->health==130);
    check(armor_draws==1);
    check(w.resources(3)->health==200);
    check(box->drain().empty());
    check(bool(publish));publish();
    const auto wire=box->drain();
    world::Packet stopped;
    world::u16(stopped,158);world::u32(stopped,59);
    world::u32(stopped,primary.actor);world::u32(stopped,0);
    check(std::count(wire.begin(),wire.end(),stopped)==1);
    stopped.clear();
    world::u16(stopped,158);world::u32(stopped,59);
    world::u32(stopped,near.actor);world::u32(stopped,0);
    check(std::count(wire.begin(),wire.end(),stopped)==0);
    unsigned armor_respecting=0,armor_ignoring=0;
    for(const auto& packet:wire)if(packet.size()>=6&&packet[0]==162&&packet[1]==0){
        const unsigned kind=unsigned(packet[2])|(unsigned(packet[3])<<8)|
            (unsigned(packet[4])<<16)|(unsigned(packet[5])<<24);
        armor_respecting+=kind==16;armor_ignoring+=kind==55;
    }
    check(armor_respecting==3&&armor_ignoring==0);
    check(!w.complete_npc_action(*primary_action,3000));
    check(w.complete_npc_action(*near_action,3000));
    auto stale=primary;++stale.generation;
    check(!w.stage_lightbringers_gaze_world(source,stale,
        *skills::effects::lightbringers_gaze(1),20,2000));
    for(unsigned protection:{16u,17u}){
        world::Instance protected_world;
        auto guarded=npcs.front();
        guarded.combat_definition->spawn_energy={20,40};
        guarded.combat_definition->energy_pips=0;
        protected_world.configure_npcs({guarded});
        auto guarded_box=std::make_shared<world::Outbox>();
        protected_world.join(4,"Protected caster",0,0,guarded_box,0,{}, {},100,1);
        protected_world.set_vitals(4,{100,100,50,50},100);
        const auto guarded_source=*protected_world.effect_scope(4);
        const auto guarded_primary=*protected_world.effect_scope(100);
        check(protected_world.npc_interrupt_stance(
            guarded_primary,protection,12,10000,900));
        const auto action=protected_world.begin_npc_action(guarded_primary,
            combat::npcs::control::Action::spell,950,3000,skills::Slot{194,0});
        check(bool(action));
        guarded_box->drain();
        std::function<void()> guarded_publish;
        check(bool(protected_world.stage_lightbringers_gaze_world(
            guarded_source,guarded_primary,
            *skills::effects::lightbringers_gaze(0),20,1000,
            &guarded_publish)));
        check(protected_world.npc_combat_snapshot(guarded_primary)->health->current==100);
        check(protected_world.complete_npc_action(*action,3000));
        check(protection!=17||
            protected_world.npc_energy_snapshot(guarded_primary)->current==15);
        check(guarded_box->drain().empty());
        check(bool(guarded_publish));guarded_publish();
        world::Packet stop;
        world::u16(stop,158);world::u32(stop,59);
        world::u32(stop,guarded_primary.actor);world::u32(stop,0);
        const auto protected_wire=guarded_box->drain();
        check(std::count(protected_wire.begin(),protected_wire.end(),stop)==0);
    }
    std::cout<<"PASS Gaze mixed damage, primary-only interrupt, stance protection, deferred packets and stale rejection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
