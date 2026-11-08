#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool ok,std::source_location where=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC external Health line "+std::to_string(where.line()));
}
static combat::npcs::LiveActor npc(double health,bool with_energy=false){
    combat::npcs::Definition definition;
    definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
    definition.relation=combat::npcs::Relation::hostile_to_players;
    definition.spawn_health=combat::Health{health,100};definition.damageable=true;
    if(with_energy){definition.spawn_energy=combat::npcs::Energy{0,100};definition.energy_pips=2;}
    return combat::npcs::LiveActor(definition,{1,1,1,146,100});
}
int main()try{
    const combat::npcs::Lifetime life{1,1,1,146,100};
    auto victim=npc(20);
    check(victim.begin_external_health(life,100));
    combat::npcs::LiveActor::ExternalHealth suffering{{1100},[](std::uint64_t at){return at<1100?0:-5;},{},42};
    const auto dead=victim.advance_conditions(life,4100,&suffering);
    check(dead&&dead->killed&&dead->death_time==3100&&dead->after.current==0&&dead->cause==42);
    check(victim.health_clock(life)==4100);
    auto stale=life;++stale.actor_generation;
    check(!victim.begin_external_health(stale,4100));

    auto ally=npc(50);
    check(ally.begin_external_health(life,100));
    combat::npcs::LiveActor::ExternalHealth blood{{1100},[](std::uint64_t at){return at<1100?5:0;},{},0};
    const auto healed=ally.advance_conditions(life,2100,&blood);
    check(healed&&healed->after.current==60&&!healed->killed);

    auto invalid=npc(50);
    check(invalid.begin_external_health(life,100));
    combat::npcs::LiveActor::ExternalHealth malformed{{1100},[](std::uint64_t at){return at<1100?5:11;},{},0};
    check(!invalid.advance_conditions(life,2100,&malformed));
    check(invalid.health_clock(life)==100&&invalid.actor(life)->health->current==50);
    auto energized=npc(100,true);
    check(energized.begin_external_health(life,100)&&energized.advance_energy(life,100));
    combat::npcs::LiveActor::ExternalHealth power{{},[](std::uint64_t){return 0;},
        [](std::uint64_t){return 2;},0};
    check(bool(energized.advance_conditions(life,3100,&power)));
    check(energized.energy(life)&&energized.energy(life)->current==4);
    std::cout<<"NPC external Health boundaries and death timing passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
