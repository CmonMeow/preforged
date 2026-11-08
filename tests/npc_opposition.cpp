#include "gw_npc_opposition.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("typed NPC opposition failed");}
combat::npcs::LiveActor actor(unsigned id,combat::npcs::Relation relation){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=4;
    d.relation=relation;d.spawn_health=combat::Health{100,100};d.damageable=relation!=combat::npcs::Relation::protected_ally;
    return {d,{1,1,1,146,id}};
}
int main()try{
    namespace o=combat::ai::opposition;
    auto ally=actor(100,combat::npcs::Relation::allied_to_players),foe=actor(101,combat::npcs::Relation::hostile_to_players);
    const auto a=ally.lifetime(),b=foe.lifetime();
    check(o::Receipt::prepare(ally,a,foe,b,{}).has_value());check(o::Receipt::prepare(foe,b,ally,a,{}).has_value());
    auto neutral=actor(102,combat::npcs::Relation::neutral),service=actor(103,combat::npcs::Relation::protected_ally);
    check(!o::Receipt::prepare(foe,b,neutral,neutral.lifetime(),{}));check(!o::Receipt::prepare(foe,b,service,service.lifetime(),{}));
    const o::QuestDomain domain{{1,1,1},46,1};o::Ownership ownership{domain,domain,domain,true,true};
    const auto receipt=o::Receipt::prepare(ally,a,foe,b,ownership);check(receipt.has_value());
    auto changed=ownership;++changed.target->cycle;check(!receipt->current(ally,a,foe,b,changed));
    changed=ownership;++changed.current->owner.generation;check(!receipt->current(ally,a,foe,b,changed));
    changed=ownership;changed.target_assigned=false;check(!receipt->current(ally,a,foe,b,changed));
    changed=ownership;changed.source.reset();check(!o::Receipt::prepare(ally,a,foe,b,changed));
    auto stale=b;++stale.actor_generation;check(!receipt->current(ally,a,foe,stale,ownership));
    const auto released=o::Receipt::prepare(foe,b,ally,a,ownership);check(released.has_value());
    check(foe.apply_damage(b,{combat::Kind::armor_ignoring,100,20,194},1000).killed);
    check(!released->current(foe,b,ally,a,ownership));check(released->current(foe,b,ally,a,ownership,true));
    const auto reverse=o::Receipt::prepare(ally,a,foe,b,ownership);check(!reverse);
    std::cout<<"NPC opposition derives actual combatant relations and exact owner/cycle without neutral relabeling\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
