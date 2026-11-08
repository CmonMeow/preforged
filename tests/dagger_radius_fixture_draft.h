#pragma once
#include "gw_world_simulation.h"
#include "dagger_retained_owner_fixture_draft.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("step2 actual dagger World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance world;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,target;
    std::unique_ptr<test::RetainedDaggerOwnerDraft> paid_owner;std::uint64_t contact_at=1000;
    combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,32,0.};
    Fixture(bool npc,unsigned hp=1000){
        world.configure_attack_miss_draws([]{return .5;});world.configure_attack_critical_draws([]{return .999999;});
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=true;
            d.physical_armor=60;d.physical_armor_evidence="test";d.fleshy=true;d.fleshy_evidence="test";
            d.knockdown_allowed=true;d.knockdown_evidence="test";n.combat_definition=d;world.configure_npcs({n});}
        world.join(1,"Assassin",0,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);
        if(!npc){world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);world.set_vitals(2,{hp,1000,30,30},100);
            target=*world.effect_scope(2);check(world.physical_armor(target,60));
            check(world.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));
        }else target=*world.effect_scope(100);
        check(world.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{7,0,20}));box->drain();
    }
    bool hit(unsigned skill,std::uint64_t at=1000,unsigned rank=0){
        const auto plan=combat::weapon::attack_skill(skill,rank,0,0,true);check(plan&&plan->required_type==32);
        if(combat::weapon::dagger_dual_role(skill)){
            if(!paid_owner)paid_owner=std::make_unique<test::RetainedDaggerOwnerDraft>(world,source,box);
            const bool accepted=paid_owner->execute(target,skill,rank,at);contact_at=paid_owner->final_contact_at();return accepted;
        }
        contact_at=at;return world.melee_attack(source,target,weapon,*plan,at,{},true);
    }
    double health()const{return target.actor==100?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
    void offhand(){check(world.gale(source,target,12,.5,1000));check(world.dagger_stage(source,target,1000)==0);
        check(hit(778));check(world.dagger_stage(source,target,1000)==2);world.take_attack_outcomes(source);box->drain();}
};
