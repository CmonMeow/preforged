#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("friendly ordinary credit World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope owner,other,source,target,unrelated;world::Instance::Npc enemy;
    combat::ai::ordinary::Reviewed weapon{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},
        "synthetic lethal ordinary profile, no inferred Rurik base damage"};
    Fixture(){
        world::Instance::Npc ally;ally.id=ally.definition.id=100;ally.definition.level=4;ally.definition.primary=1;
        ally.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=4;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{100,100};d.damageable=true;
        ally.combat_definition=d;w.configure_npcs({ally});
        for(unsigned id:{1u,2u}){w.join(id,"Owner",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{100,100,20,20},100);}
        owner=*w.effect_scope(1);other=*w.effect_scope(2);source=*w.effect_scope(100);
        enemy.id=enemy.definition.id=101;enemy.definition.level=4;enemy.definition.visual_adjustment={0,0,0,100};enemy.x=100;
        d.binding.actor=d.binding.definition=101;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1,1};d.physical_armor=0;
        enemy.combat_definition=d;
        check(spawn(owner,1));target=*w.effect_scope(60000);check(spawn(other,1));unrelated=*w.effect_scope(60001);
        check(w.configure_npc_friendly_ordinary(source,weapon,1000,world::Instance::FriendlyOrdinaryQuest{owner,75,1}));
    }
    bool spawn(skills::effects::Scope who,unsigned cycle){return w.spawn_kill_encounter(who,75,cycle,{enemy},[]{return true;},false,{},{},false,0,false,{},{},1);}
};
int main()try{
    {Fixture f;check(!f.w.begin_npc_friendly_ordinary(f.source,f.unrelated,1000));
        const auto token=f.w.begin_npc_friendly_ordinary(f.source,f.target,1000);check(token.has_value());f.w.tick(token->completes);
        check(f.w.npc_combat_snapshot(f.target)->health->current==0);check(f.w.npc_combat_snapshot(f.unrelated)->health->current==1);
        const auto credit=f.w.pending_quest_credits(f.owner);check(credit.size()==1&&credit[0].quest==75&&credit[0].cycle==1);
        check(f.w.pending_quest_credits(f.other).empty());check(f.w.take_npc_deaths(f.owner).empty()); //no fabricated Player kill source
        f.w.tick(token->completes+100);check(f.w.pending_quest_credits(f.owner).size()==1);
        check(f.w.acknowledge_quest_credit(f.owner,credit[0]));check(!f.w.acknowledge_quest_credit(f.owner,credit[0]));
        check(f.w.clear_kill_objective(f.owner,75,1,true));check(f.spawn(f.owner,2));const auto fresh=*f.w.effect_scope(60002);
        check(!f.w.begin_npc_friendly_ordinary(f.source,fresh,3000));
        check(f.w.configure_npc_friendly_ordinary(f.source,f.weapon,3000,world::Instance::FriendlyOrdinaryQuest{f.owner,75,2}));
        check(f.w.begin_npc_friendly_ordinary(f.source,fresh,3000).has_value());}
    {Fixture f;const auto token=f.w.begin_npc_friendly_ordinary(f.source,f.target,1000);check(token.has_value());
        check(f.w.clear_kill_objective(f.owner,75,1,false));check(f.spawn(f.owner,2));
        f.w.tick(token->completes);check(f.w.npc_combat_snapshot(f.target)->health->current==1);check(f.w.pending_quest_credits(f.owner).empty());
        check(!f.w.npc_ordinary_pending(f.source));}
    {Fixture f;const auto token=f.w.begin_npc_friendly_ordinary(f.source,f.target,1000);check(token.has_value());
        const auto old=f.owner;f.w.leave(1);f.w.join(1,"Replacement",200,0,std::make_shared<world::Outbox>(),0,{}, {},1100,1);
        f.w.set_vitals(1,{100,100,20,20},100);f.owner=*f.w.effect_scope(1);check(f.owner!=old);check(f.spawn(f.owner,1));
        const auto fresh=*f.w.effect_scope(60002);f.w.tick(token->completes);
        check(f.w.npc_combat_snapshot(fresh)->health->current==1&&f.w.pending_quest_credits(f.owner).empty());
        check(!f.w.begin_npc_friendly_ordinary(f.source,fresh,3000));
        check(f.w.configure_npc_friendly_ordinary(f.source,f.weapon,3000,world::Instance::FriendlyOrdinaryQuest{f.owner,75,1}));}
    std::cout<<"Allied ordinary kills retain exact owner/cycle credits without Player fabrication\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
