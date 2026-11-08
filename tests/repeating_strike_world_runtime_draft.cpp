// Unregistered candidate-only test: needs paid-candidate Runtime/World headers.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_echo_commit.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("paid dagger candidate line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance world;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,target;combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,32,0.};
    skills::casting::Runtime cast{1};skills::casting::Hooks hooks;skills::Policy policy;skills::Slots bar{};
    attributes::Ranks ranks{};std::uint64_t clock=1000;
    unsigned activity_callbacks=0,attack_activity_callbacks=0,active_copy=0,skill=0,payments=0,paid=0,admissions=0,completions=0,refunds=0;
    std::uint64_t last_use_event=0;unsigned copied_observations=0;std::optional<effects::CooldownCompletion> copied_completion;
    combat::weapon::AttackPermit last_permit;combat::weapon::DaggerPaidCompletionDraft last_result;
    Fixture(unsigned id,bool npc=false):skill(id){
        world.configure_attack_miss_draws([]{return .5;});world.configure_attack_critical_draws([]{return .999999;});
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
            d.damageable=d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="paid fixture";
            d.fleshy=true;d.fleshy_evidence="paid fixture";d.knockdown_allowed=true;d.knockdown_evidence="paid fixture";
            n.combat_definition=d;world.configure_npcs({n});}
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
int main()try{
 for(bool npc:{false,true}){
  Fixture f(976,npc);f.arm();check(!skills::casting::Runtime::effect_supported(976));
  for(unsigned repeat=0;repeat<2;++repeat){
   check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();
   f.update(at);check(f.last_result.accepted&&!f.last_result.prerequisite_failed&&f.last_result.outcomes.size()==1);
   check(f.last_result.outcomes[0].outcome==combat::weapon::AttackOutcome::hit&&f.until()==0);
   check(f.world.dagger_stage(f.source,f.target,at)==2);
   const auto before=f.health();const auto plan=*combat::weapon::attack_skill(976,12,0,0,true);
   const auto replay=f.world.complete_dagger_cast_draft(f.source,f.target,f.weapon,plan,f.last_permit,at);
   check(!replay.accepted&&f.health()==before);f.clock=at+1;
  }
  check(f.payments==2&&f.paid==10&&f.refunds==0);
 }
 for(bool block:{false,true}){
  Fixture f(976);f.arm();
  if(block){check(f.world.status_mutation(f.target,259,50,12,1000,9000,true));f.world.configure_attack_block_draws([]{return 0.;});}
  else{check(f.world.apply_condition(f.target,f.source,combat::conditions::Kind::blind,1000,10000));f.world.configure_attack_miss_draws([]{return 0.;});}
  check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();const auto hp=f.health();f.update(at);
  check(f.last_result.accepted&&f.last_result.outcomes.size()==1&&!f.last_result.outcomes[0].skill_failed);
  check(f.last_result.outcomes[0].outcome==(block?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::miss));
  check(f.health()==hp&&f.until()==(block?0:at+15000)&&f.payments==1&&f.paid==5&&f.refunds==0);
  const auto receipt=f.world.cooldown_completion_stamp(f.source,f.last_use_event);check(bool(receipt));
  check(receipt->slot==skills::Slot{976,0}&&receipt->base_ms==0&&receipt->extra_ms==(block?0u:15000u));
  check(effects::completion_deadline(*receipt)==(block?at:at+15000));
  const auto copied=*receipt;check(effects::completion_deadline(copied)==effects::completion_deadline(*receipt));
  check(f.copied_observations==1&&f.copied_completion==receipt);
  check(effects::completion_deadline(*f.copied_completion)==(block?at:at+15000));
  check(!f.world.finalize_diversion_skill_use(f.source,f.last_use_event,at,0,0,false));
  check(f.world.cooldown_completion_stamp(f.source,f.last_use_event)==receipt);
  const auto snapshot=f.cast.cooldown_source_snapshot(f.source,at,f.hooks);check(bool(snapshot));
  check(f.world.publish_local_cooldowns(f.source,*snapshot));
  if(!block){check(snapshot->timers.size()==1&&snapshot->timers[0].origin&&snapshot->timers[0].origin->token==f.last_use_event&&snapshot->timers[0].until==at+15000);}
  f.update(at);check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 }
 {Fixture f(976);check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  check(f.last_result.prerequisite_failed&&f.last_result.outcomes[0].skill_failed&&f.until()==0&&f.health()==1000&&f.paid==5);}
 {Fixture f(976);f.arm();check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();
  check(f.world.prepare_diversion_skill_use(f.source,{976,0},900000,at));
  const auto correct=*combat::weapon::attack_skill(976,12,0,0,true);
  check(!f.world.complete_dagger_cast_draft(f.source,f.target,f.weapon,correct,f.last_permit,at,1).accepted);
  check(f.last_permit->open());
  const auto wrong=*combat::weapon::attack_skill(976,12,0,1,true);
  check(!f.world.complete_dagger_cast_draft(f.source,f.target,f.weapon,wrong,f.last_permit,at,1).accepted);
  check(f.last_permit->open());check(f.world.abort_diversion_skill_use(f.source,900000));f.update(at);check(f.last_result.accepted&&f.paid==5);}
 {Fixture f(976);f.arm();check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();
  f.world.leave(2);f.world.join(2,"NewFoe",60,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);f.world.set_vitals(2,{1000,1000,30,30},100);
  f.update(at);check(!f.last_permit->open()&&f.world.resources(2)->health==1000&&f.paid==5);}
 {Fixture f(976);f.arm();check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();
  auto replacement=f.weapon;++replacement.revision;check(f.world.ordinary_weapon(f.source,replacement,20,1100,combat::weapon::CriticalProfile{7,12,20}));
  const auto before=f.health();f.update(at);check(!f.last_result.accepted&&f.health()==before&&f.paid==5);}
 {Fixture f(976);f.arm();check(f.start()==skills::casting::Reason::accepted);f.cast.cancel(f.hooks,1100);
  check(!f.last_permit->open()&&f.until()==0&&f.paid==5&&f.completions==0);}
 {Fixture f(976);f.arm();check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();
  check(f.world.gale(f.target,f.source,0,.5,1100));const auto before=f.health();f.update(at);
  check(!f.last_permit->open()&&f.health()==before&&f.paid==5&&f.refunds==0);}
 // Genuine World-issued Echo copy, TemporarySkillSession grant and Runtime admission.
 {Fixture f(976);f.arm();f.policy.secondary=5;f.policy.learned[74]=1;f.bar[1]={74,0};
  check(f.world.configure_adrenaline(f.source,f.bar,1000));
  effects::TemporarySkillSession session(f.source,f.bar);effects::TemporarySkillIds ids(f.source);
  const auto echo=*effects::echo(74);check(f.world.status_mutation(f.source,74,100,0,1000,1000+echo.armed_duration,true,skills::Slot{74,0}));
  check(f.start()==skills::casting::Reason::accepted);const auto first=f.last_permit->release_time();f.update(first);
  const auto copied=effects::commit_echo_copy(f.world,session,ids,f.source,{74,0},{976,0},first,f.last_use_event);
  check(copied&&copied->changed&&copied->changed->after==skills::Slot{976,1});f.bar=session.bar();f.active_copy=1;
  f.hooks.admit_session_cast=[&](const auto& bar,const auto& policy,auto request,auto at)->std::optional<effects::HexCopySession::CastAdmission>{
   if(bar!=session.bar())return {};const auto found=std::find(bar.begin(),bar.end(),skills::Slot{request.skill,request.copy});
   if(found==bar.end())return {};return session.admit(f.source,policy,unsigned(found-bar.begin()),{request.skill,request.copy},at);
  };
  f.hooks.valid_session_cast=[&](const auto& admission,auto at){return session.valid(admission,f.policy,at);};
  f.clock=first+1;check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,f.clock,10000));f.world.configure_attack_miss_draws([]{return 0.;});
  check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();const auto before=f.health();f.update(at);
  const auto receipt=f.world.cooldown_completion_stamp(f.source,f.last_use_event);
  check(receipt&&receipt->slot==skills::Slot{976,1}&&receipt->extra_ms==15000&&effects::completion_deadline(*receipt)==at+15000);
  check(f.copied_completion==receipt&&f.until()==at+15000&&f.health()==before&&f.paid==10&&f.refunds==0);
  const auto plan=*combat::weapon::attack_skill(976,12,0,1,true);
  check(!f.world.complete_dagger_cast_draft(f.source,f.target,f.weapon,plan,f.last_permit,at,1).accepted);
  const auto observed=f.copied_observations;f.update(at);check(f.copied_observations==observed);
  // Expired grant must cancel even when optional disable authority is unavailable.
  f.clock=first+echo.copy_duration-200;f.arm();check(f.start()==skills::casting::Reason::accepted);
  const auto expiry_release=f.last_permit->release_time();const auto hp=f.health();const auto commits=f.completions;
  f.hooks.skill_disabled_slot=[](auto,auto)->std::optional<std::uint64_t>{return {};};f.box->drain();f.update(expiry_release);
  check(!f.cast.active()&&!f.last_permit->open()&&f.health()==hp&&f.completions==commits&&f.paid==15);
  const auto expired=f.box->drain();check(std::count(expired.begin(),expired.end(),skills::cancellation_packet(1,{976,1}))==1);
 }
 for(bool explicit_cancel:{false,true}){Fixture f(976);f.arm();check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();
  f.world.leave(1);f.world.join(1,"NewSource",0,0,f.box,0,{}, {},1100,1);f.world.set_vitals(1,{1000,1000,30,30},100);
  check(!f.last_permit->open()); // World retirement revokes retained capability immediately.
  const auto hp=f.health();const auto resources=*f.world.resources(1);const auto activity=f.activity_callbacks,attack_activity=f.attack_activity_callbacks;
  f.box->drain();if(explicit_cancel)f.cast.cancel(f.hooks,at);else f.update(at);
  check(!f.cast.active()&&!f.last_permit->open()&&f.health()==hp&&f.paid==5&&f.completions==0);
  check(f.activity_callbacks==activity&&f.attack_activity_callbacks==attack_activity&&f.box->drain().empty());
  const auto after=*f.world.resources(1);check(after.health==resources.health&&after.energy==resources.energy);}
 // Unavailable disable query on SAME owner is retry, not proof of retirement.
 {Fixture f(976);f.arm();check(f.start()==skills::casting::Reason::accepted);const auto at=f.last_permit->release_time();
  const auto authority=f.hooks.skill_disabled_slot;f.hooks.skill_disabled_slot=[](auto,auto)->std::optional<std::uint64_t>{return {};};
  f.update(at);check(f.cast.active()&&f.last_permit->open()&&f.completions==0);
  f.hooks.skill_disabled_slot=authority;f.update(at);check(!f.cast.active()&&f.last_result.accepted&&f.paid==5);}
 std::cout<<"PASS detached actual World/Runtime Repeating976 owner permits and outcome recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
