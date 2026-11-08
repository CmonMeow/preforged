// Shared ordinary NPC-recipient release/impact regression. No quest46 stats implied.
#include "gw_world_simulation.h"
#include "gw_npc_ordinary_recipient.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("friendly ordinary World failed");}
int main()try{
    world::Instance w;std::vector<world::Instance::Npc> population;
    for(unsigned id:{100u,101u}){
        world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=4;n.definition.primary=3;
        n.definition.visual_adjustment={0,0,0,100};n.x=id==100?0:100;
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=4;
        d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{100,100};d.physical_armor=0;d.damageable=d.conditions_allowed=true;
        d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
        d.healing_allowed=true;d.healing_evidence="synthetic reviewed friendly source";n.combat_definition=d;population.push_back(n);
    }
    w.configure_npcs(std::move(population));w.join(1,"Observer ally",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.set_vitals(1,{50,100,40,40},100);const auto source=*w.effect_scope(100),target=*w.effect_scope(101),player=*w.effect_scope(1);
    check(w.configure_npc_friendly_healer(source,{286,0,0,"synthetic known primary, explicit rank/resource"}));
    const combat::ai::ordinary::Reviewed weapon{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},
        "synthetic5damage midpoint/no-critical source; not captured retail base damage"};
    check(w.configure_npc_friendly_ordinary(source,weapon,1000));
    const auto heal=w.begin_npc_friendly_heal(source,player,1000);check(heal.has_value());
    check(!w.begin_npc_friendly_ordinary(source,target,1100));w.tick(heal->completes);
    check(!w.begin_npc_friendly_ordinary(source,target,heal->completes));
    const auto ready=heal->completes+static_cast<std::uint64_t>(std::ceil(skills::find_skill(286)->aftercast*1000));
    const auto attack=w.begin_npc_friendly_ordinary(source,target,ready);check(attack.has_value());
    check(!w.begin_npc_friendly_ordinary(source,target,ready));
    w.tick(attack->completes);check(w.npc_combat_snapshot(target)->health->current<100);
    check(w.npc_energy_snapshot(source)->current==30); //ordinary attack adds no duplicate resource owner/spend
    const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==1&&outcomes.front().outcome==combat::weapon::AttackOutcome::hit);
    std::cout<<"Friendly ordinary NPC contact shares source control, healer aftercast and target Health\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
