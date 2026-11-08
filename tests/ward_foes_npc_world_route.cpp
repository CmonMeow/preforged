#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("Ward177 live NPC route line "+std::to_string(where.line()));
}
int main()try{
    world::Instance w;
    world::Instance::Npc guardian;
    guardian.id=guardian.definition.id=100;
    guardian.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition definition;
    definition.binding.map=146;
    definition.binding.actor=definition.binding.definition=100;
    definition.relation=combat::npcs::Relation::neutral;
    definition.spawn_health=combat::Health{1000,1000};
    definition.damageable=definition.conditions_allowed=true;
    definition.creature_type="human";
    definition.creature_type_evidence="authored fixture";
    guardian.combat_definition=definition;
    w.configure_npcs({guardian});
    auto output=std::make_shared<world::Outbox>();
    w.join(2,"Caster",0,0,output,0,{}, {},100,1);
    w.set_vitals(2,{1000,1000,100,100},100);
    const auto caster=*w.effect_scope(2),guard=*w.effect_scope(100);
    auto enemy=guardian;
    enemy.x=400;
    enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
    check(w.begin_wave_objective(caster,guard,73,1,3,1));
    check(w.spawn_quest_wave(caster,73,1,{enemy},[]{return true;}));
    const auto foe=*w.effect_scope(60000);
    const auto plan=skills::effects::ward_foes_plan(0);
    check(plan&&w.apply_ward_foes(caster,*plan,100));
    check(w.chase_quest_guardian(foe,100,100));
    const auto history=w.ward_foes_history(foe,100,2100);
    check(history&&history->size()==1);
    w.tick(2100);
    // Starting 400 away: 78 units outside the 322-unit field at 100/s,
    // followed by 61 units inside at 50/s. Sparse tick must land at 261.
    check(!w.projectile_target_valid(caster,60000,260,2100));
    check(w.projectile_target_valid(caster,60000,262,2100));
    world::Instance sparse;
    sparse.configure_npcs({guardian});
    sparse.join(2,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    sparse.set_vitals(2,{1000,1000,100,100},100);
    const auto sparse_caster=*sparse.effect_scope(2),sparse_guard=*sparse.effect_scope(100);
    check(sparse.begin_wave_objective(sparse_caster,sparse_guard,73,1,3,1));
    check(sparse.spawn_quest_wave(sparse_caster,73,1,{enemy},[]{return true;}));
    const auto sparse_foe=*sparse.effect_scope(60000);
    check(sparse.apply_ward_foes(sparse_caster,*plan,100));
    check(sparse.chase_quest_guardian(sparse_foe,50,100));
    sparse.tick(9100); // Field expired at 8100, after most of the travel.
    // 78 outside at 50/s, then 161 inside at 25/s, then 50 at 50/s.
    check(!sparse.projectile_target_valid(sparse_caster,60000,110,9100));
    check(sparse.projectile_target_valid(sparse_caster,60000,112,9100));
    std::cout<<"Ward177 live quest NPC route entry and sparse tick passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
