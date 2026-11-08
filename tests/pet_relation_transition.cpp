#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace combat::npcs;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("pet relation line "+std::to_string(at.line()));
}
int main()try{
    Definition d;d.binding={162,100,50,128406,0,0,13,5};d.relation=Relation::neutral;
    d.spawn_health=combat::Health{123.5,180};d.damageable=true;d.conditions_allowed=true;d.packed_allegiance=123456;
    d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=35;
    d.healing_allowed=true;d.healing_evidence="Animal companion healing wiki and Comfort capture";
    d.knockdown_allowed=true;d.knockdown_evidence="synthetic reviewed revival fixture";
    const Lifetime life{1,1,4,162,100};LiveActor actor(d,life);
    using Step=LiveActor::StalkerCharmTransition;
    auto stale=life;++stale.actor_generation;
    auto wrong=d.binding;++wrong.model_file;
    check(!actor.transition_stalker_charm(stale,d.binding,Step::hostility,100));
    check(!actor.transition_stalker_charm(life,wrong,Step::hostility,100));
    check(!actor.transition_stalker_charm(life,d.binding,Step::adoption,100));
    const auto old=*actor.begin_action(life,control::Action::attack,100,1000);
    const auto hostile=actor.transition_stalker_charm(life,d.binding,Step::hostility,101);
    check(hostile&&hostile->cancelled==old&&!actor.active_action(life,102)&&actor.hostile_to_players(life));
    check(actor.actor(life)->health->current==123.5&&actor.packed_allegiance(life)==123456);
    check(actor.apply_condition(life,combat::conditions::Kind::blind,2,102,10000));
    const auto epoch=actor.control_epoch(life);
    check(!actor.transition_stalker_charm(life,d.binding,Step::hostility,103)&&actor.control_epoch(life)==epoch);
    const auto adopted=actor.transition_stalker_charm(life,d.binding,Step::adoption,103);
    check(adopted&&actor.healable_ally(life)&&!actor.hostile_to_players(life)&&!actor.neutral_actor(life));
    check(actor.actor(life)->health->current==123.5&&actor.has_condition(life,combat::conditions::Kind::blind,104));
    check(actor.lifetime()==life&&actor.packed_allegiance(life)==123456);
    check(!actor.transition_stalker_charm(life,d.binding,Step::adoption,104));
    const auto pre_death=*actor.begin_action(life,control::Action::attack,105,1000);
    check(actor.knock_down(life,110,2000)==control::Knockdown::accepted);
    actor.actor(life)->health->current=0;actor.actor(life)->effects|=0x10;
    const skills::effects::Scope owner{1,2,3};
    check(!actor.revive_owned_stalker(stale,d.binding,owner,48,111));
    check(actor.revive_owned_stalker(life,d.binding,owner,48,111));
    check(actor.actor(life)->health->current==86&&actor.actor(life)->health->maximum==180);
    check(!(actor.actor(life)->effects&0x10)&&!actor.knocked_down(life,112)&&!actor.has_condition(life,combat::conditions::Kind::blind,112));
    check(actor.lifetime()==life&&actor.packed_allegiance(life)==123456);
    const auto revived=*actor.begin_action(life,control::Action::attack,112,1112);
    check(revived.epoch>pre_death.epoch&&revived.sequence>pre_death.sequence&&!actor.complete_action(life,pre_death,1112));
    check(!actor.revive_owned_stalker(life,d.binding,owner,100,1113));
    auto dead=LiveActor(d,life);dead.actor(life)->health->current=0;
    check(!dead.transition_stalker_charm(life,d.binding,Step::hostility,100));
    auto service=d;service.relation=Relation::protected_ally;service.damageable=false;service.conditions_allowed=false;
    auto protected_actor=LiveActor(service,life);check(!protected_actor.transition_stalker_charm(life,d.binding,Step::hostility,100));
    std::cout<<"Exact Stalker charm relation transitions preserve combat lifetime/state\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
