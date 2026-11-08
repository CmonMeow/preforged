#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("opposed ordinary World line "+std::to_string(at.line()));
}
world::Instance::Npc make_test_npc(unsigned id,combat::npcs::Relation relation,unsigned armor=0){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=4;n.definition.visual_adjustment={0,0,0,100};n.x=id==100?0:100;
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=4;d.relation=relation;
    d.spawn_health=combat::Health{100,100};d.damageable=relation!=combat::npcs::Relation::protected_ally;
    d.physical_armor=armor;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;n.combat_definition=d;return n;
}
int main()try{
    const combat::ai::ordinary::Reviewed weapon{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},
        "synthetic explicit two-side weapon profile, not retail tuning"};
    {world::Instance w;w.configure_npcs({make_test_npc(100,combat::npcs::Relation::allied_to_players,60),make_test_npc(101,combat::npcs::Relation::hostile_to_players)});
        w.join(1,"Observer",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);const auto ally=*w.effect_scope(100),enemy=*w.effect_scope(101);
        check(w.configure_npc_opposed_ordinary(ally,weapon,1000));check(w.configure_npc_opposed_ordinary(enemy,weapon,1000));
        check(w.begin_npc_opposed_ordinary(ally,enemy,1000).has_value());check(w.begin_npc_opposed_ordinary(enemy,ally,1000).has_value());
        w.tick(1750);const auto ally_health=w.npc_combat_snapshot(ally)->health->current,enemy_health=w.npc_combat_snapshot(enemy)->health->current;
        check(ally_health<100&&enemy_health<100&&ally_health>enemy_health); //known armor applies to actual allied recipient
        check(w.npc_energy_snapshot(ally)->current==40&&w.npc_energy_snapshot(enemy)->current==40);
        check(w.take_attack_outcomes(ally).size()==1&&w.take_attack_outcomes(enemy).size()==1);}
    {world::Instance w;w.configure_npcs({make_test_npc(100,combat::npcs::Relation::allied_to_players),make_test_npc(101,combat::npcs::Relation::hostile_to_players),
            make_test_npc(102,combat::npcs::Relation::protected_ally),make_test_npc(103,combat::npcs::Relation::neutral)});
        w.join(1,"Observer",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);const auto enemy=*w.effect_scope(101);
        check(w.configure_npc_opposed_ordinary(enemy,weapon,1000));
        check(!w.begin_npc_opposed_ordinary(enemy,*w.effect_scope(102),1000));check(!w.begin_npc_opposed_ordinary(enemy,*w.effect_scope(103),1000));
        check(!w.configure_npc_opposed_ordinary(*w.effect_scope(102),weapon,1000));}
    {world::Instance w;w.configure_npcs({make_test_npc(100,combat::npcs::Relation::allied_to_players),make_test_npc(101,combat::npcs::Relation::allied_to_players)});
        for(unsigned id:{1u,2u}){w.join(id,"Owner",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{100,100,20,20},100);}
        const auto owner=*w.effect_scope(1),other=*w.effect_scope(2),ally=*w.effect_scope(100),unrelated=*w.effect_scope(101);
        auto enemy_template=make_test_npc(110,combat::npcs::Relation::hostile_to_players);
        check(w.spawn_kill_encounter(owner,75,1,{enemy_template},[]{return true;}));
        check(w.spawn_kill_encounter(other,75,1,{enemy_template},[]{return true;}));const auto enemy=*w.effect_scope(60000);
        const std::array own_allies{ally},other_allies{unrelated};
        check(w.bind_npc_opposed_allies({owner,75,1},own_allies));check(w.bind_npc_opposed_allies({other,75,1},other_allies));
        check(w.configure_npc_opposed_ordinary(enemy,weapon,1000,world::Instance::FriendlyOrdinaryQuest{owner,75,1}));
        check(!w.begin_npc_opposed_ordinary(enemy,unrelated,1000));check(w.begin_npc_opposed_ordinary(enemy,ally,1000).has_value());
        w.tick(1750);check(w.npc_combat_snapshot(ally)->health->current<100&&w.npc_combat_snapshot(unrelated)->health->current==100);}
    std::cout<<"Actual hostile/allied NPC opposition shares ordinary tokens, armor and exact owned domains\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
