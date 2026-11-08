#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Signet of Rejuvenation World line "+std::to_string(at.line()));
}
int main()try{
    const auto plan=*skills::effects::signet_rejuvenation(0);
    for(unsigned mode=0;mode<4;++mode){
        world::Instance world;
        const auto box=std::make_shared<world::Outbox>();
        world.join(1,"Caster",0,0,box,0,{}, {},100,1);
        world.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        world.join(3,"Foe",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})world.set_vitals(id,{20,100,50,50},100);
        const auto source=*world.effect_scope(1),ally=*world.effect_scope(2);
        check(!world.stage_signet_rejuvenation_player(source,*world.effect_scope(3),plan,200));
        auto stale=ally;++stale.generation;
        check(!world.stage_signet_rejuvenation_player(source,stale,plan,200));
        if(mode==1)check(world.cast_activity(ally,281,100,1000)); // spell
        if(mode==2)check(world.cast_activity(ally,887,100,1000)); // signet, not spell
        if(mode==3)check(world.attack_activity(ally,500,100));
        box->drain();
        std::function<void()> publish;
        check(world.stage_signet_rejuvenation_player(source,ally,plan,200,&publish));
        const unsigned gained=(mode==1||mode==3)?30:15;
        check(world.resources(2)->health==20+gained);
        const auto packet=skills::particles::health_change_2022(2,1,gained,100);
        const auto premature=box->drain();
        check(std::find(premature.begin(),premature.end(),packet)==premature.end());
        check(bool(publish));publish();
        const auto delivered=box->drain();
        check(std::find(delivered.begin(),delivered.end(),packet)!=delivered.end());
    }
    for(unsigned mode=0;mode<4;++mode){
        world::Instance world;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=60;
        npc.definition.level=20;npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;
        body.binding.map=146;body.binding.actor=body.binding.definition=100;
        body.binding.level=20;body.spawn_health={20,100};body.damageable=true;
        body.relation=mode==3?combat::npcs::Relation::hostile_to_players:
            combat::npcs::Relation::allied_to_players;
        body.healing_allowed=true;body.healing_evidence="fixture";
        npc.combat_definition=body;
        world.configure_npcs({npc});
        const auto box=std::make_shared<world::Outbox>();
        world.join(1,"Caster",0,0,box,0,{}, {},100,1);
        world.set_vitals(1,{100,100,50,50},100);
        const auto source=*world.effect_scope(1),ally=*world.effect_scope(100);
        if(mode==1)check(bool(world.begin_npc_action(ally,
            combat::npcs::control::Action::attack,150,500)));
        if(mode==2)check(bool(world.begin_npc_action(ally,
            combat::npcs::control::Action::spell,150,500,skills::Slot{281,0})));
        box->drain();
        std::function<void()> publish;
        const bool accepted=world.stage_signet_rejuvenation_npc(
            source,ally,plan,200,&publish);
        check(accepted==(mode!=3));
        const unsigned gained=mode==0?15:30;
        check(world.npc_combat_snapshot(ally)->health->current==
              (accepted?20+gained:20));
        if(accepted){
            const auto packet=skills::particles::health_change_2022(100,1,gained,100);
            const auto premature=box->drain();
            check(std::find(premature.begin(),premature.end(),packet)==premature.end());
            check(bool(publish));publish();
            const auto delivered=box->drain();
            check(std::find(delivered.begin(),delivered.end(),packet)!=delivered.end());
        }
    }
    std::cout<<"PASS Signet of Rejuvenation player/NPC targets, action snapshot and deferred heal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
