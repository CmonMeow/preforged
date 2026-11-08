#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Shields Up NPC line "+std::to_string(at.line()));
}
static combat::npcs::Definition definition(combat::npcs::Relation relation){
    combat::npcs::Definition d;
    d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
    d.relation=relation;d.spawn_health=combat::Health{500,500};
    d.physical_armor=60;d.earth_armor=60;d.fire_armor=60;
    d.damageable=true;d.conditions_allowed=true;return d;
}
int main()try{
    const combat::npcs::Lifetime life{1,1,1,146,100};
    const skills::effects::Scope caster{1,1,1};
    const auto plan=*skills::effects::shields_up(12);
    for(const auto relation:{combat::npcs::Relation::hostile_to_players,
            combat::npcs::Relation::neutral,combat::npcs::Relation::allied_to_players}){
        combat::npcs::LiveActor npc(definition(relation),life);
        const auto normal=npc.final_physical_armor(life,100);
        check(normal==60&&npc.final_caster_armor(life,11,100)==60);
        check(!npc.apply_shields_up(life,caster,plan,7,100,false,0));
        check(!npc.apply_shields_up(life,caster,plan,7,100,true,1012.01f));
        auto stale=life;++stale.actor_generation;
        check(!npc.apply_shields_up(stale,caster,plan,7,100,true,0));
        if(relation!=combat::npcs::Relation::allied_to_players){
            check(!npc.apply_shields_up(life,caster,plan,7,100,true,0));continue;
        }
        const auto applied=npc.apply_shields_up(life,caster,plan,7,100,true,1012.f);
        check(applied&&applied->accepted&&applied->removed.empty());
        check(npc.shields_up_armor(life,100)==60);
        check(npc.final_physical_armor(life,100)==60);
        check(npc.final_physical_armor(life,100,true)==120);
        check(npc.final_caster_armor(life,11,100)==60);
        check(npc.final_caster_armor(life,11,100,true)==120);
        check(npc.final_caster_armor(life,5,100,true)==120);
        check(npc.final_physical_armor(life,100+plan.duration,true)==60);
        check(npc.shields_up_armor(life,100+plan.duration)==0);
        const auto replacement=npc.apply_shields_up(life,caster,plan,8,200,true,100);
        check(replacement&&replacement->removed.size()==1&&
            replacement->removed.front().instance==7);
        check(npc.shields_up_armor(life,200)==60);
        check(!npc.apply_shields_up(life,caster,plan,8,300,true,100));
    }
    std::cout<<"Shields Up NPC recipient admission, projectile armor before conditions, replacement and expiry pass; World party ownership remains gated\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
