// Explicitly enrolled synthetic hold-position selector in normal World.
#include "gw_world_simulation.h"
#include "gw_npc_friendly_heal_selection.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("autonomous friendly healing World failed");}
int main()try{
    world::Instance w;std::vector<world::Instance::Npc> population;
    for(unsigned id:{100u,101u}){
        world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=5;n.definition.primary=3;
        n.definition.visual_adjustment={0,0,0,100};n.x=id==100?0:100;
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=5;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{id==100?200.:150.,200};
        d.damageable=true;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
        d.healing_allowed=true;d.healing_evidence="synthetic authored PvE ally fixture";n.combat_definition=d;population.push_back(n);
    }
    w.configure_npcs(std::move(population));w.join(1,"Injured ally",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.set_vitals(1,{60,100,40,40},100);const auto source=*w.effect_scope(100),other=*w.effect_scope(101);
    check(w.configure_npc_friendly_healer(source,{286,7,6,"reviewed AcademyMonk ranks, explicit synthetic resources"}));
    const combat::ai::friendly::HealPolicy policy{800,combat::ai::friendly::HealPriority::lowest_fraction,true,0,
        "authored80percent lowest-fraction hold-position fixture, not retail target priority"};
    check(w.configure_npc_friendly_heal_ai(source,policy));
    w.tick_npc_friendly_heal_ai(1000);check(w.npc_energy_snapshot(source)->current==30);
    w.tick_npc_friendly_heal_ai(1000);check(w.npc_energy_snapshot(source)->current==30);
    const auto end=1000+static_cast<std::uint64_t>(std::ceil(skills::find_skill(286)->activation*1000));
    w.tick(end);check(w.resources(1)->health==100);check(w.npc_combat_snapshot(other)->health->current==150);
    w.tick_npc_friendly_heal_ai(end);check(w.npc_energy_snapshot(source)->current==30); //aftercast/recharge still owned by source book
    const auto ready=end+static_cast<std::uint64_t>(std::ceil(std::max(skills::find_skill(286)->aftercast,
        float(skills::find_skill(286)->recharge))*1000))+1;
    w.tick_npc_friendly_heal_ai(ready);check(w.npc_energy_snapshot(source)->current==20);
    w.tick(ready+(end-1000));check(w.npc_combat_snapshot(other)->health->current==200);
    std::cout<<"Autonomous authored friendly selector feeds one real scoped healing queue\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
