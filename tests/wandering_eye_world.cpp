#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <tuple>
#include <utility>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Wandering Eye World application line "+std::to_string(at.line()));
}
static std::vector<skills::effects::Entry> effects(world::Instance& world,
        skills::effects::Scope scope,std::uint64_t at){
    auto entries=world.status_snapshot(scope,at);check(bool(entries));return *entries;
}
int main()try{
    for(bool npc_target:{false,true}){
        world::Instance world;
        if(npc_target){
            world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=60;
            npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition body;body.binding.map=146;
            body.binding.actor=body.binding.definition=100;
            body.relation=combat::npcs::Relation::hostile_to_players;
            body.spawn_health=combat::Health{500,500};body.damageable=true;
            body.conditions_allowed=true;npc.combat_definition=body;
            world.configure_npcs({npc});
        }
        world.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        world.set_vitals(1,{1000,1000,30,30},100);
        if(!npc_target){
            world.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            world.set_vitals(2,{1000,1000,30,30},100);
        }
        const auto caster=*world.effect_scope(1),target=*world.effect_scope(npc_target?100:2);
        check(!world.apply_wandering_eye({caster.instance,caster.actor,caster.generation+1},target,12,200));
        check(!world.apply_wandering_eye(caster,{target.instance,target.actor,target.generation+1},12,200));
        check(effects(world,target,200).empty());
        check(world.apply_wandering_eye(caster,target,12,200));
        const auto first=effects(world,target,200);check(first.size()==1&&first[0].skill==2056&&
            first[0].rank==12&&first[0].cause==caster.actor&&first[0].expires==4200);
        check(world.apply_wandering_eye(caster,target,12,300));
        const auto renewed=effects(world,target,300);check(renewed.size()==1&&
            renewed[0].skill==2056&&renewed[0].instance!=first[0].instance&&
            renewed[0].expires==4300);
        check(effects(world,target,4300).empty());
    }
    {
        world::Instance world;
        world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=85;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;body.binding.map=146;
        body.binding.actor=body.binding.definition=100;
        body.relation=combat::npcs::Relation::hostile_to_players;
        body.spawn_health=combat::Health{500,500};body.damageable=true;
        npc.combat_definition=body;world.configure_npcs({npc});
        for(auto [id,x,team]:{std::tuple{1u,0.f,1u},{2u,60.f,2u},
                {3u,100.f,2u},{4u,90.f,1u},{5u,500.f,2u}}){
            world.join(id,"Player",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,team);
            world.set_vitals(id,{1000,1000,30,30},100);
        }
        const auto caster=*world.effect_scope(1),hexed=*world.effect_scope(2);
        check(world.apply_wandering_eye(caster,hexed,12,200));
        check(!world.attack_activity(hexed,1000,300)); // One-shot interruption.
        check(effects(world,hexed,300).empty());
        check(world.resources(2)->health==906&&world.resources(3)->health==906);
        check(world.resources(4)->health==1000&&world.resources(5)->health==1000);
        check(world.npc_combat_snapshot(*world.effect_scope(100))->health->current==406);
        const auto target_health=world.resources(2)->health;
        check(world.attack_activity(hexed,1000,400));
        check(world.resources(2)->health==target_health); // Cannot fire twice.
    }
    for(bool protected_npc:{false,true}){
        world::Instance world;
        std::vector<world::Instance::Npc> npcs;
        for(auto [id,x]:{std::pair{100u,60.f},std::pair{101u,90.f},std::pair{102u,500.f}}){
            world::Instance::Npc npc;npc.id=npc.definition.id=id;npc.x=x;
            npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition body;body.binding.map=146;
            body.binding.actor=body.binding.definition=id;
            body.relation=combat::npcs::Relation::hostile_to_players;
            body.spawn_health=combat::Health{500,500};body.damageable=true;
            npc.combat_definition=body;npcs.push_back(npc);
        }
        world.configure_npcs(npcs);
        for(auto [id,x,team]:{std::tuple{1u,0.f,1u},{2u,80.f,1u},
                {3u,100.f,2u},{4u,500.f,2u}}){
            world.join(id,"Player",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,team);
            world.set_vitals(id,{1000,1000,30,30},100);
        }
        const auto caster=*world.effect_scope(1),hexed=*world.effect_scope(100);
        check(world.apply_wandering_eye(caster,hexed,12,200));
        if(protected_npc)check(world.npc_interrupt_stance(hexed,16,12,10000,250));
        const auto token=world.begin_npc_action(hexed,combat::npcs::control::Action::attack,300,900);
        check(bool(token));
        check(effects(world,hexed,300).empty());
        check(world.npc_combat_snapshot(hexed)->health->current==406);
        check(world.npc_combat_snapshot(*world.effect_scope(101))->health->current==406);
        check(world.npc_combat_snapshot(*world.effect_scope(102))->health->current==500);
        check(world.resources(1)->health==1000&&world.resources(2)->health==1000&&
              world.resources(3)->health==906&&world.resources(4)->health==1000);
        check(world.complete_npc_action(*token,900)==protected_npc);
        const auto after=world.npc_combat_snapshot(hexed)->health->current;
        check(bool(world.begin_npc_action(hexed,combat::npcs::control::Action::attack,1000,1600)));
        check(world.npc_combat_snapshot(hexed)->health->current==after); // One shot.
    }
    {
        world::Instance world;
        for(auto [id,team]:{std::pair{1u,1u},std::pair{2u,2u}}){
            world.join(id,"Player",id==1?0.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,team);
            world.set_vitals(id,{1000,1000,30,30},100);
        }
        const auto caster=*world.effect_scope(1),hexed=*world.effect_scope(2);
        check(world.apply_wandering_eye(caster,hexed,12,200));
        world.leave(1);
        world.join(1,"Replacement",90,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        world.set_vitals(1,{1000,1000,30,30},100);
        check(!world.attack_activity(hexed,1000,300));
        check(world.resources(2)->health==906);
        check(world.resources(1)->health==906); // Recycled ID is a foe, never the credited caster.
        check(effects(world,hexed,300).empty());
        check(world.attack_activity(hexed,1000,400));
        check(world.resources(2)->health==906);
    }
    {
        world::Instance world;
        world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=60;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;body.binding.map=146;
        body.binding.actor=body.binding.definition=100;
        body.relation=combat::npcs::Relation::hostile_to_players;
        body.spawn_health=combat::Health{500,500};body.damageable=true;
        npc.combat_definition=body;world.configure_npcs({npc});
        world.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        world.set_vitals(1,{1000,1000,30,30},100);
        const auto caster=*world.effect_scope(1),hexed=*world.effect_scope(100);
        check(world.apply_wandering_eye(caster,hexed,12,200));
        world.leave(1);
        const auto token=world.begin_npc_action(hexed,combat::npcs::control::Action::attack,300,900);
        check(bool(token));
        check(world.npc_combat_snapshot(hexed)->health->current==406);
        check(effects(world,hexed,300).empty());
        check(!world.complete_npc_action(*token,900));
    }
    std::cout<<"PASS Wandering Eye World player/NPC application and both attack-start owners\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
