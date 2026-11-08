#include "gw_ward_foes_npc_route.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
using namespace skills::effects;
using namespace combat::npcs;
void check(bool value){if(!value)throw std::runtime_error("Ward177 NPC route");}
bool near(double a,double b){return std::abs(a-b)<.01;}
int main()try{
    Definition definition;
    definition.binding.map=146;
    definition.binding.actor=definition.binding.definition=100;
    definition.binding.level=20;
    definition.relation=Relation::hostile_to_players;
    definition.spawn_health=combat::Health{100,100};
    definition.damageable=definition.conditions_allowed=true;
    definition.spawn_energy=Energy{100,100};
    definition.knockdown_allowed=true;
    definition.knockdown_evidence="authored fixture";
    const Lifetime lifetime{1,1,2,146,100};
    LiveActor actor(definition,lifetime);
    const auto ward=*ward_against_foes({1,9,1},1,0,0,0,0,0);
    const std::array wards{ward};
    const WardFoeRecipient recipient{actor.hex_scope(),0,0,0,true,false,true};
    const std::array route{WardFoeRouteLeg{0,0,1000,0,0}};
    auto travel=ward_foes_npc_route_travel(recipient,wards,route,actor,lifetime,0,1000,288);
    check(travel&&near(travel->distance,144)&&near(travel->x,144));
    check(actor.configure_spellbook(lifetime,{{{194,0},1248,"movement history fixture"}}));
    const combat::ai::SpellTarget target{{1,9,1},100,true,true,true,true};
    const auto cast=actor.begin_spell(lifetime,{194,0},target,100);
    check(cast&&cast->completes==1100&&actor.complete_action(lifetime,*cast,1100));
    travel=ward_foes_npc_route_travel(recipient,wards,route,actor,lifetime,0,2000,288);
    check(travel&&near(travel->distance,36));
    auto stale=lifetime;++stale.actor_generation;
    check(!ward_foes_npc_route_travel(recipient,wards,route,actor,stale,0,2000,288));
    actor.consume_movement(lifetime,1000);
    check(!ward_foes_npc_route_travel(recipient,wards,route,actor,lifetime,0,2000,288));
    std::cout<<"Ward177 NPC route composition and history rejection passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
