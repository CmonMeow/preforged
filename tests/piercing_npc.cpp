#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Piercing NPC line "+std::to_string(at.line()));
}
int main()try{
    const combat::npcs::Lifetime life{1,1,1,146,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};
    d.damageable=true;d.earth_armor=100;
    combat::npcs::LiveActor unknown(d,life);
    const combat::Damage bite{combat::Kind::piercing,70,20,2213};
    check(unknown.apply_damage(life,bite,100).reason==combat::Reason::unknown_armor);
    check(unknown.actor(life)->health->current==500);
    d.physical_armor=60;d.physical_armor_evidence="test";
    combat::npcs::LiveActor known(d,life);
    const auto hit=known.apply_damage(life,bite,100);
    check(hit.reason==combat::Reason::accepted&&hit.damage==70);
    check(known.actor(life)->health->current==430);
    check(!combat::damage_packets_2022(1,hit).empty());
    std::cout<<"NPC piercing uses physical armor and publishes a hit\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
