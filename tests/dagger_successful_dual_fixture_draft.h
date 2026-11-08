#pragma once
// Unregistered candidate-only test: needs paid-candidate Runtime/World headers.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_echo_commit.h"
#include "gw_inspired_hex_commit.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error(std::string("paid dagger candidate ")+at.file_name()+":"+std::to_string(at.line())+" in "+at.function_name());
}
struct Fixture {
    world::Instance world;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,target;combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,32,0.};
    skills::casting::Runtime cast{1};skills::casting::Hooks hooks;skills::Policy policy;skills::Slots bar{};
    attributes::Ranks ranks{};std::uint64_t clock=1000;
    unsigned activity_callbacks=0,attack_activity_callbacks=0,active_copy=0,skill=0,payments=0,paid=0,admissions=0,completions=0,refunds=0;
    std::uint64_t last_use_event=0;unsigned copied_observations=0;std::optional<effects::CooldownCompletion> copied_completion;
    std::vector<combat::weapon::ResolvedAttack> second_outcomes;
    combat::weapon::AttackPermit last_permit;combat::weapon::DaggerPaidCompletionDraft last_result;
    Fixture(unsigned id,bool npc=false,bool unknown_ally=false):skill(id){
        world.configure_attack_miss_draws([]{return .5;});world.configure_attack_critical_draws([]{return .999999;});
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
            d.damageable=d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="paid fixture";
            d.fleshy=true;d.fleshy_evidence="paid fixture";d.knockdown_allowed=true;d.knockdown_evidence="paid fixture";
            n.combat_definition=d;world.configure_npcs({n});}
        if(unknown_ally){world::Instance::Npc n;n.id=n.definition.id=101;n.x=30;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding={146,101,101,0,0,0,0,20};d.relation=combat::npcs::Relation::allied_to_players;
            d.spawn_health=combat::Health{500,1000};d.damageable=true;d.creature_type="spirit";d.creature_type_evidence="unknown Seed gain cohort";
            d.fleshy=false;d.fleshy_evidence="authored spirit";n.combat_definition=d;world.configure_npcs({n});}
        world.join(1,"Assassin",0,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);
        check(world.energy_capacity(1,7,0,100,{10,-2}));
        if(!npc){world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            world.set_vitals(2,{1000,1000,30,30},100);target=*world.effect_scope(2);check(world.physical_armor(target,60));
            check(world.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));
        }else target=*world.effect_scope(100);
        check(world.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{7,12,20}));
        policy.primary=7;policy.campaigns=255;policy.learned[id]=1;bar[0]={static_cast<std::uint16_t>(id),0};
        ranks[29]=12;ranks[35]=12;ranks[17]=0;
        hooks.resources=[&](unsigned actor){return world.resources(actor);};
        hooks.events=[&](const auto& packets){box->enqueue(packets);};
        hooks.target_scope=[&](unsigned actor){return world.effect_scope(actor);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){++payments;paid+=cost;return world.consume_energy(1,cost,source,at);};
        hooks.restore_energy=[&](unsigned amount){++refunds;return world.restore_energy(1,amount,source,clock);};
        hooks.weapon_snapshot=[&]{return world.attack_weapon(source);};
        hooks.melee_target=[&](unsigned actor,auto at){return world.melee_target(source,actor,at,skill);};
        hooks.weapon_attack_timing=[&](auto at){return world.weapon_attack_timing(source,at);};
        hooks.attack_duration=[&](auto base,auto at){return world.attack_duration(source,base,at,true);};
        hooks.attack_activity=[&](auto until){++attack_activity_callbacks;world.attack_activity(source,until,clock);};
        hooks.control_epoch=[&]{return world.control_epoch(source);};
        hooks.prepare_dagger_attack=[&](auto target_scope,const auto& gear,const auto& plan,auto began,auto release){
            ++admissions;check(plan.skill==skill&&plan.rank==12&&plan.strength==0&&plan.required_type==32);
            check(skills::find_skill(skill)->attribute==(skill==776||skill==779||skill==783?35u:29u));
            last_permit=world.prepare_dagger_cast_draft(source,target_scope,gear,plan,began,release);return last_permit;
        };
        hooks.complete_dagger_successful_second=[&](const auto& phase,const auto& before,auto at){auto result=world.complete_dagger_successful_second_draft(phase,before,at);second_outcomes=result.outcomes;return result;};
        hooks.cooldown_amendment_source=[&](auto event){return world.cooldown_amendment_source(source,event);};
        hooks.complete_dagger_failed_second=[&](const auto& phase,auto at){return world.complete_dagger_failed_second_draft(phase,at);};
        hooks.complete_dagger_attack=[&](auto target_scope,const auto& gear,const auto& plan,const auto& permit,auto at,unsigned copy){
            ++completions;last_result=world.complete_dagger_cast_draft(source,target_scope,gear,plan,permit,at,copy);
            if(last_result.failure_feedback){
                auto foreign=source;++foreign.generation;
                check(!last_result.failure_feedback->consume(foreign,target_scope,plan.skill,copy,permit));
                foreign=target_scope;++foreign.generation;
                check(!last_result.failure_feedback->consume(source,foreign,plan.skill,copy,permit));
                check(!last_result.failure_feedback->consume(source,target_scope,plan.skill,copy+1,permit));
                check(!last_result.failure_feedback->consume(source,target_scope,plan.skill,copy,{}));
            }
            if(last_result.dual_failure_feedback){
                auto wrong=source;++wrong.generation;
                check(!last_result.dual_failure_feedback->consume(0,wrong,target_scope,plan.skill,copy,permit,at));
                check(!last_result.dual_failure_feedback->consume(0,source,target_scope,plan.skill,copy+1,permit,at));
                check(!last_result.dual_failure_feedback->consume(1,source,target_scope,plan.skill,copy,permit,at+500)); // phase0 not consumed
            }
            return last_result;
        };
        check(world.configure_adrenaline(source,bar,100));
        hooks.cast_slot_activity=[&](auto slot,auto begins,auto ends){++activity_callbacks;const bool changed=world.cast_activity(source,slot,begins,ends);if(world.effect_scope(source.actor)==source)check(changed);else check(!changed);};
        hooks.prepare_skill_use=[&](auto slot,auto event,auto at){last_use_event=event;return world.prepare_diversion_skill_use(source,slot,event,at);};
        hooks.finalize_skill_use=[&](auto event,auto at,auto ordinary,auto existing,bool instant){return world.finalize_diversion_skill_use(source,event,at,ordinary,existing,instant);};
        hooks.abort_skill_use=[&](auto event){return world.abort_diversion_skill_use(source,event);};
        hooks.cooldown_completion_source=[&](auto event){return world.cooldown_completion_stamp(source,event);};
        hooks.cooldown_control_source=[&](auto after){return world.cooldown_control_source(source,after);};
        hooks.skill_disabled_slot=[&](auto slot,auto at){return world.skill_disabled_slot(source,slot,at);};
        hooks.current_skillbar=[&]()->std::optional<skills::Slots>{return bar;};
        hooks.completed_skill_use=[&](const effects::CooldownCompletion& receipt){
            const auto actual=world.cooldown_completion_stamp(source,receipt.skill_use);
            check(actual&&*actual==receipt);++copied_observations;copied_completion=receipt;return true;
        };
        box->drain();
    }
    auto start(){return cast.start(clock,{skill,active_copy,target.actor},bar,policy,ranks,true,hooks);}
    void update(std::uint64_t at){clock=at;cast.update(clock,hooks);}
    double health()const{return target.actor==100?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
    void arm(){
        // Clean current stage2: no Poison/Bleeding or target knockdown.
        check(world.iron_palm(source,target,*effects::iron_palm(0),20,clock));
        const auto fox=*combat::weapon::attack_skill(780,0,0,0,true);
        check(world.melee_attack(source,target,weapon,fox,clock,{},true));
        check(world.dagger_stage(source,target,clock)==2);
        world.take_attack_outcomes(source);box->drain();
    }
    std::uint64_t until(){const auto snapshot=cast.cooldown_source_snapshot(source,clock,hooks);check(bool(snapshot));
        for(const auto& timer:snapshot->timers)if(timer.slot==skills::Slot{static_cast<std::uint16_t>(skill),active_copy})return timer.until;
        return 0;
    }
};
