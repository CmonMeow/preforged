// Unregistered candidate-only test: needs paid-candidate Runtime/World headers.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
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
    unsigned skill=0,payments=0,paid=0,admissions=0,completions=0,refunds=0;
    combat::weapon::AttackPermit last_permit;combat::weapon::DaggerPaidCompletionDraft last_result,second_result;
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
        hooks.attack_activity=[&](auto until){world.attack_activity(source,until,clock);};
        hooks.control_epoch=[&]{return world.control_epoch(source);};
        hooks.prepare_dagger_attack=[&](auto target_scope,const auto& gear,const auto& plan,auto began,auto release){
            ++admissions;check(plan.skill==skill&&plan.rank==12&&plan.strength==0&&plan.required_type==32);
            check(skills::find_skill(skill)->attribute==(skill==776||skill==779||skill==783?35u:29u));
            last_permit=world.prepare_dagger_cast_draft(source,target_scope,gear,plan,began,release);return last_permit;
        };
        hooks.complete_dagger_successful_second=[&](const auto& phase,const auto& before,auto at){second_result=world.complete_dagger_successful_second_draft(phase,before,at);return second_result;};
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
        if(combat::weapon::dagger_dual_role(skill)){
        hooks.prepare_skill_use=[&](auto slot,auto event,auto at){return world.prepare_diversion_skill_use(source,slot,event,at);};
        hooks.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool zero){return world.finalize_diversion_skill_use(source,event,at,base,existing,zero);};
        hooks.abort_skill_use=[&](auto event){return world.abort_diversion_skill_use(source,event);};
        hooks.cooldown_completion_source=[&](auto event){return world.cooldown_completion_stamp(source,event);};
        hooks.cooldown_control_source=[&](auto after){return world.cooldown_control_source(source,after);};
        hooks.skill_disabled_slot=[&](auto slot,auto at){return world.skill_disabled_slot(source,slot,at);};
        hooks.current_skillbar=[&]()->std::optional<skills::Slots>{return bar;};
        check(world.configure_adrenaline(source,bar,100));
        }
        box->drain();
    }
    auto start(){return cast.start(clock,{skill,0,target.actor},bar,policy,ranks,true,hooks);}
    void update(std::uint64_t at){clock=at;cast.update(clock,hooks);}
    double health()const{return target.actor==100?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
    void arm(){
        if(skill==780)check(world.iron_palm(source,target,*effects::iron_palm(0),20,clock));
        if(skill==778||skill==775||skill==776||skill==777)check(world.gale(source,target,12,.5,clock));
        if(skill==775||skill==776||skill==777){const auto spider=*combat::weapon::attack_skill(778,0,0,0,true);
            check(world.melee_attack(source,target,weapon,spider,clock,{},true));}
        world.take_attack_outcomes(source);box->drain();
    }
    std::uint64_t until(){const auto snapshot=cast.cooldown_source_snapshot(source,clock,hooks);check(bool(snapshot));
        for(const auto& timer:snapshot->timers)if(timer.slot==skills::Slot{static_cast<std::uint16_t>(skill),0})return timer.until;
        return 0;
    }
};
int main()try{
    for(unsigned skill:{775u,776u,777u,778u,779u,780u,782u,783u})for(bool npc:{false,true}){
        Fixture f(skill,npc);f.arm();check(!skills::casting::Runtime::effect_supported(skill));
        check(f.start()==skills::casting::Reason::accepted);check(f.payments==1&&f.paid==skills::find_skill(skill)->energy_cost());
        check(f.last_permit&&f.last_permit->open());const auto release=f.last_permit->release_time();
        check(release==1000+(skill==780||skill==782?250u:500u));
        check(skills::find_skill(780)->activation==.5f&&skills::find_skill(782)->activation==.5f);
        check(skills::find_skill(skill)->aftercast==0);
        f.update(release-1);check(f.completions==0&&f.cast.active());
        f.update(release);check(f.completions==1&&f.cast.active()==(skill<=777)&&!f.last_permit->open());
        check(f.payments==1&&f.refunds==0&&f.last_result.accepted&&!f.last_result.prerequisite_failed);
        check(f.last_result.outcomes.size()==1);
        if(skill<=777){check(bool(f.last_result.successful_second));f.update(f.last_result.successful_second->due());
            check(!f.cast.active()&&f.second_result.accepted&&f.second_result.outcomes.size()==1&&f.payments==1);}
        check(f.until()==release+skills::find_skill(skill)->recharge*1000ull);
        const auto before=f.health();f.update(f.clock);check(f.completions==1&&f.health()==before);
        const auto plan=*combat::weapon::attack_skill(skill,12,0,0,true);
        const auto replay=f.world.complete_dagger_cast_draft(f.source,f.target,f.weapon,plan,f.last_permit,release);
        check(!replay.accepted&&replay.outcomes.empty()&&f.health()==before);
        check(!skills::casting::Runtime::effect_supported(skill));
    }
    for(unsigned skill:{780u,776u,778u}){
        Fixture f(skill);const auto before=f.world.resources(1)->energy;
        check(f.start()==skills::casting::Reason::accepted&&f.payments==1&&f.completions==0);
        check(f.world.resources(1)->energy==before-skills::find_skill(skill)->energy_cost());
        const auto release=f.last_permit->release_time();f.update(release);
        check(f.last_result.accepted&&f.last_result.prerequisite_failed&&f.last_result.reset_recharge&&f.until()==0);
        auto failed=f.world.take_attack_outcomes(f.source);check(failed.size()==1);
        check(failed[0].outcome==combat::weapon::AttackOutcome::cancelled&&failed[0].skill_failed&&!failed[0].before_damage);
        if(skill==776){f.update(release+500);failed=f.world.take_attack_outcomes(f.source);check(failed.size()==1&&
            failed[0].outcome==combat::weapon::AttackOutcome::cancelled&&failed[0].skill_failed&&!failed[0].before_damage);}
        check(f.refunds==0&&f.health()==1000);
    }
    for(bool block:{false,true}){
        Fixture f(782);if(block){const auto guardian=*skills::regeneration::reviewed(258,12);
            check(f.world.apply_health_regeneration(f.target,2,guardian,100,100+guardian.duration,1248,true,f.target));
            f.world.configure_attack_block_draws([]{return 0.;});}
        else{check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));f.world.configure_attack_miss_draws([]{return 0.;});}
        check(f.start()==skills::casting::Reason::accepted);f.update(1250);
        check(f.payments==1&&f.paid==5&&f.refunds==0&&f.last_result.outcomes.size()==1);
        check(f.last_result.outcomes[0].outcome==(block?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::miss));
        check(f.until()==2250&&f.health()==1000);
    }
    {
        Fixture f(780);f.arm();check(f.start()==skills::casting::Reason::accepted);
        f.world.join(3,"Other foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.world.set_vitals(3,{1000,1000,30,30},100);
        check(f.world.iron_palm(f.source,*f.world.effect_scope(3),*effects::iron_palm(0),20,1100));
        f.update(1250);check(f.last_result.accepted&&f.last_result.prerequisite_failed&&f.last_result.reset_recharge);
        check(f.last_result.outcomes.size()==1&&f.payments==1&&f.paid==5&&f.refunds==0&&f.until()==0);
        check(bool(f.last_result.failure_feedback));
        const auto wire=*combat::weapon::dagger_failure_wire_draft(f.source,f.target);
        const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),wire)==1);
        check(!f.last_result.failure_feedback->consume(f.source,f.target,780,0,f.last_permit));
    }
    {
        Fixture f(778);check(f.world.gale(f.source,f.target,12,.5,1000));f.clock=2900;
        check(f.start()==skills::casting::Reason::accepted);f.update(3400);
        check(f.last_result.accepted&&f.last_result.prerequisite_failed&&f.last_result.reset_recharge);
        check(f.last_result.outcomes.size()==1&&f.payments==1&&f.refunds==0&&f.until()==0);
        check(bool(f.last_result.failure_feedback));
        const auto wire=*combat::weapon::dagger_failure_wire_draft(f.source,f.target);
        const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),wire)==1);
        check(!f.last_result.failure_feedback->consume(f.source,f.target,778,0,f.last_permit));
    }
    {
        Fixture f(782);check(f.start()==skills::casting::Reason::accepted);f.clock=1100;f.cast.cancel(f.hooks,f.clock);
        check(!f.cast.active()&&!f.last_permit->open()&&f.until()==0&&f.completions==0);
        check(f.start()==skills::casting::Reason::accepted);f.clock=1150;f.cast.interrupt(f.clock,f.hooks);
        check(!f.cast.active()&&!f.last_permit->open()&&f.until()==2150&&f.payments==2&&f.completions==0);
        check(f.world.take_attack_outcomes(f.source).empty());
    }
    {
        Fixture f(782);check(f.start()==skills::casting::Reason::accepted);
        f.world.leave(2);f.world.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);
        f.world.set_vitals(2,{1000,1000,30,30},100);f.update(1250);
        check(!f.cast.active()&&f.completions==0&&!f.last_permit->open()&&f.world.resources(2)->health==1000);
    }
    {
        Fixture f(782);check(f.start()==skills::casting::Reason::accepted);auto replacement=f.weapon;++replacement.revision;
        check(f.world.ordinary_weapon(f.source,replacement,20,1100,combat::weapon::CriticalProfile{7,12,20}));f.update(1250);
        check(f.completions==1&&!f.last_result.accepted&&f.last_result.outcomes.empty()&&f.health()==1000);
    }
    {
        Fixture f(782);check(f.world.status_mutation(f.source,346,70,0,1000,9000,true));
        const auto duration=f.world.attack_duration(f.source,250,1000,true);check(duration&&*duration==168);
        check(f.start()==skills::casting::Reason::accepted&&f.last_permit->release_time()==1168);f.update(1168);check(f.completions==1);
    }
    {
        Fixture f(782);f.hooks.complete_dagger_attack={};check(f.start()==skills::casting::Reason::unsupported&&f.payments==0);
    }
    {
        Fixture f(782);check(f.start()==skills::casting::Reason::accepted);f.update(1250);
        f.skill=779;f.policy.learned[779]=1;f.bar[1]={779,0};
        check(f.start()==skills::casting::Reason::accepted&&f.payments==2); // zero aftercast; other slot's cooldown independent
        f.cast.cancel(f.hooks,f.clock);
    }
    for(unsigned skill:{775u,776u,777u})for(unsigned mode=0;mode<5;++mode){
        Fixture f(skill);f.arm();check(f.start()==skills::casting::Reason::accepted);
        const auto release=f.last_permit->release_time();
        f.world.join(3,"Other foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        f.world.set_vitals(3,{1000,1000,30,30},100);
        check(f.world.iron_palm(f.source,*f.world.effect_scope(3),*effects::iron_palm(0),20,1100));
        f.box->drain();f.update(release);
        check(f.last_result.accepted&&f.last_result.prerequisite_failed&&f.last_result.outcomes.size()==1);
        check(f.last_result.dual_failure_feedback&&f.cast.active()&&f.until()==0&&f.payments==1&&f.refunds==0);
        const auto wire=*combat::weapon::dagger_failure_wire_draft(f.source,f.target);
        const auto first=f.box->drain();check(std::count(first.begin(),first.end(),wire)==1);
        check(std::find(first.begin(),first.end(),skills::casting::terminal(46,1))!=first.end());
        check(std::find(first.begin(),first.end(),skills::casting::terminal(47,1))==first.end());
        check(std::find(first.begin(),first.end(),skills::cancellation_packet(1,{static_cast<std::uint16_t>(skill),0}))==first.end());
        check(!f.last_result.dual_failure_feedback->consume(0,f.source,f.target,skill,0,f.last_permit,release));
        check(!f.last_result.dual_failure_feedback->consume(1,f.source,f.target,skill,0,f.last_permit,release+499));
        if(mode==1)f.cast.cancel(f.hooks,release+1);
        if(mode==2)f.cast.interrupt(release+1,f.hooks);
        if(mode==3){f.world.leave(2);f.world.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},release+1,2);}
        if(mode==4){f.world.leave(1);f.world.join(1,"Replacement",0,0,f.box,0,{}, {},release+1,1);}
        f.box->drain();f.update(release+499);check(f.box->drain().empty());
        f.update(release+500);const auto published=f.box->drain();
        // World legitimately settles the offhand's Poison during phase2.
        // Preserve exact owned failure-frame order without counting unrelated
        // condition/resource publication as another failure receipt.
        std::vector<world::Packet> second;
        for(const auto& packet:published)if(packet==skills::casting::terminal(47,1)||packet==wire||
            packet==skills::cancellation_packet(1,{static_cast<std::uint16_t>(skill),0}))second.push_back(packet);
        if(mode==0){
            check(second.size()==3&&second[0]==skills::casting::terminal(47,1)&&second[1]==wire&&
                  second[2]==skills::cancellation_packet(1,{static_cast<std::uint16_t>(skill),0}));
        }else check(second.empty());
        check(!f.cast.active()&&f.payments==1&&f.refunds==0);
        f.update(release+1000);check(f.box->drain().empty());
        check(!f.last_result.dual_failure_feedback->consume(1,f.source,f.target,skill,0,f.last_permit,release+1000));
        const auto actual_failed=f.world.take_attack_outcomes(f.source);
        if(mode!=4)check(actual_failed.size()==(mode==0?2u:1u));
        for(const auto& event:actual_failed)check(event.outcome==combat::weapon::AttackOutcome::cancelled&&event.skill_failed&&!event.before_damage);
        if(mode==0)check(actual_failed[0].event!=actual_failed[1].event); // real distinct unqualified releases
    }
    for(unsigned skill:{780u,778u,775u,776u,777u}){
        Fixture f(skill);check(f.world.spiteful_spirit(f.target,f.source,*effects::spiteful_spirit(0),20,1000));
        check(f.start()==skills::casting::Reason::accepted);const auto release=f.last_permit->release_time();
        f.update(release);check(f.world.resources(1)->health==995&&f.health()==1000);
        if(skill<=777){f.update(release+500);check(f.world.resources(1)->health==995&&f.health()==1000);}
        check(f.payments==1&&f.refunds==0&&f.until()==0);
    }
    for(unsigned skill:{780u,778u,775u,776u,777u}){
        Fixture f(skill);unsigned misses=0,blocks=0;
        f.world.configure_attack_miss_draws([&]{++misses;return 0.;});f.world.configure_attack_block_draws([&]{++blocks;return 0.;});
        skills::Slots iwbar{};iwbar[0]={33,0};check(f.world.configure_adrenaline(f.source,iwbar,1000));
        check(f.world.status_mutation(f.source,33,123,12,1000,31000,true)); // IW must not replace an invalid sequence
        check(f.world.apply_insidious_parasite(f.target,f.source,12,1000));
        f.world.set_vitals(1,{10,1000,30,30},100);
        check(f.start()==skills::casting::Reason::accepted);const auto release=f.last_permit->release_time();f.update(release);
        if(skill<=777)f.update(release+500);
        check(f.world.resources(1)->health==10&&f.health()==1000&&misses==0&&blocks==0);
        for(const auto& event:f.world.take_attack_outcomes(f.source))check(event.outcome==combat::weapon::AttackOutcome::cancelled&&event.skill_failed&&!event.before_damage);
    }
    for(unsigned skill:{780u,778u,775u,776u,777u}){
        Fixture f(skill);check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));
        check(f.world.apply_price_of_failure(f.target,f.source,0,1000));
        f.world.set_vitals(1,{100,1000,30,30},100);
        check(f.start()==skills::casting::Reason::accepted);const auto release=f.last_permit->release_time();f.update(release);
        check(f.world.resources(1)->health==99); // failed attack Price1; NO Vigorous healing
        if(skill<=777){f.update(release+500);check(f.world.resources(1)->health==98);}
    }
    std::cout<<"PASS candidate paid eight-dagger scopes, one fee, actual dual events, timing, cancellation and failure reset policy; normal bindings remain unsupported\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
