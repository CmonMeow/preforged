// Regression for the reviewed World primary-profession guard.
#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("NPC Divine enrollment failed");}
int main()try{
    for(unsigned primary:{0u,1u,3u}){
        world::Instance w;std::vector<world::Instance::Npc> population;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=5;
            n.definition.primary=static_cast<std::uint8_t>(primary);n.definition.visual_adjustment={0,0,0,100};n.x=id==100?0:100;
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=5;
            d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{id==100?500.:50.,500};
            d.damageable=true;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
            d.healing_allowed=true;d.healing_evidence="synthetic recipient healing authority";n.combat_definition=d;population.push_back(n);
        }
        w.configure_npcs(std::move(population));w.join(1,"Observer",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        const auto source=*w.effect_scope(100),target=*w.effect_scope(101);
        const combat::ai::friendly::HealProfile profile{286,7,6,"AcademyMonk wiki ranks, synthetic known primary/resource fixture"};
        check(w.configure_npc_friendly_healer(source,profile)==(primary==3));
        if(primary!=3){
            auto no_divine=profile;no_divine.divine=0;check(w.configure_npc_friendly_healer(source,no_divine));
        }
        const auto token=w.begin_npc_friendly_heal(source,target,1000);check(token.has_value());w.tick(token->completes);
        const auto amount=*skills::effects::direct_heal_amount(286,7,primary==3?6:0,50,500);
        check(w.npc_combat_snapshot(target)->health->current==50+amount);
        check(w.npc_energy_snapshot(source)->current==30);
    }
    std::cout<<"Known Monk primary gates NPC Divine Favor without fabricating other profession bonuses\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
