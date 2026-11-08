#include "gw_world_simulation.h"
#include "dagger_retained_owner_fixture_draft.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error(std::string("step2 actual dagger World ")+at.file_name()+":"+std::to_string(at.line())+" in "+at.function_name());
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
int main()try{
    for(unsigned skill:{775u,776u,777u,778u,779u,780u,782u,783u}){
        Fixture f(false);const auto owned=combat::weapon::attack_skill(skill,0,0,0,true);check(bool(owned));
        check(!combat::weapon::attack_skill(skill,0)&&!combat::weapon::valid(*owned));
        check(combat::weapon::valid(*owned,true));
        check(!f.world.melee_attack(f.source,f.target,f.weapon,*owned,1000)); // default World owner=false
        check(f.health()==1000&&f.world.take_attack_outcomes(f.source).empty());
        auto forged=*owned;++forged.bonus;
        check(!combat::weapon::valid(forged,true));
        check(!f.world.melee_attack(f.source,f.target,f.weapon,forged,1000,{},true));
        check(f.health()==1000&&f.world.take_attack_outcomes(f.source).empty());
    }
    for(bool npc:{false,true})for(unsigned hp:{900u,901u}){
        Fixture f(npc,hp);check(f.hit(783));check(f.health()==hp-(hp==900?39u:54u));
        check(f.world.dagger_stage(f.source,f.target,1000)==1);
    }
    for(bool npc:{false,true})for(bool hexed:{false,true}){
        Fixture f(npc);f.world.set_vitals(1,{1000,1000,20,30},100);
        if(hexed)check(f.world.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,0),1000));
        check(f.hit(779)&&f.health()==970);check(f.world.resources(1)->energy==(hexed?25u:20u));
    }
    for(bool npc:{false,true}){
        Fixture f(npc);check(!f.hit(778)&&f.health()==1000&&f.world.take_attack_outcomes(f.source).empty());
        f.offhand();check(f.health()==965&&f.world.has_condition(f.target,combat::conditions::Kind::poison,1000));
        check(f.hit(776));const auto events=f.world.take_attack_outcomes(f.source);
        check(events.size()==2&&events[0].event!=events[1].event);
        for(const auto& event:events)check(event.outcome==combat::weapon::AttackOutcome::hit&&event.before_damage==event.at);
        check(f.world.has_condition(f.target,combat::conditions::Kind::bleeding,f.contact_at));
        check(f.world.has_condition(f.target,combat::conditions::Kind::deep_wound,f.contact_at));
        check(f.world.dagger_stage(f.source,f.target,f.contact_at)==3);
    }
    {
        Fixture f(false);f.world.join(3,"Collateral",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        f.world.set_vitals(3,{1000,1000,30,30},100);f.offhand();check(f.hit(775));
        check(f.health()==877&&f.world.resources(3)->health==960); // two primary40 + Poison8 over1s, two collateral20
        check(f.world.take_attack_outcomes(f.source).size()==2);
    }
    for(bool npc:{false,true})for(bool ally:{false,true}){
        Fixture f(npc);f.offhand();
        if(ally){
            // Player target team2 has an authored nearby team2 ally. NPC ally
            // classification needs packed allegiance fixture; it is not guessed.
            if(npc)continue;
            f.world.join(3,"Target ally",80,0,std::make_shared<world::Outbox>(),0,{}, {},1000,2);
            f.world.set_vitals(3,{1000,1000,30,30},100);
        }
        check(f.hit(777,4000));check(f.world.knocked_down(f.target,f.contact_at)==!ally);
        check(f.world.take_attack_outcomes(f.source).size()==2);
    }
    for(bool npc:{false,true})for(bool block:{false,true}){
        Fixture f(npc);f.offhand();unsigned draws=0;
        if(block){
            if(npc)check(f.world.stage_npc_shield_deflection(f.target,*effects::shield_deflection(12),1000));
            else check(f.world.status_mutation(f.target,259,50,12,1000,9000,true));
            f.world.configure_attack_block_draws([&]{return draws++==0?0.:.999999;});
        }else{check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1000,5000));
            f.world.configure_attack_miss_draws([&]{return draws++==0?0.:.999999;});}
        check(f.hit(776));const auto events=f.world.take_attack_outcomes(f.source);check(events.size()==2);
        check(events[0].outcome==(block?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::miss));
        check(!events[0].before_damage&&events[1].outcome==combat::weapon::AttackOutcome::hit&&events[1].before_damage==events[1].at);
        check(events[0].event!=events[1].event&&f.world.dagger_stage(f.source,f.target,f.contact_at)==3);
    }
    for(bool npc:{false,true}){
        Fixture f(npc,50);f.offhand();check(f.health()==15&&f.hit(776));check(f.health()==0);
        const auto events=f.world.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::hit);
    }
    {
        Fixture f(false);
        // Apply before Gale: the target is our hostile hex caster and cannot
        // cast while the offhand fixture's knockdown is active.
        check(f.world.apply_insidious_parasite(f.target,f.source,12,1000));
        f.offhand();f.world.set_vitals(1,{10,1000,30,30},100);
        const bool fatal_contact_accepted=f.hit(776);
        if(!fatal_contact_accepted)std::cerr<<"fatal dual accepted=false source_health="<<f.world.resources(1)->health<<" outcomes_pending\n";
        check(fatal_contact_accepted);
        check(f.paid_owner&&f.paid_owner->payments==1&&f.paid_owner->paid==skills::find_skill(776)->energy_cost()&&f.paid_owner->completions==1);
        check(!f.paid_owner->first_result.successful_second&&f.paid_owner->second_result.outcomes.empty());
        check(f.world.resources(1)->health==0);const auto events=f.world.take_attack_outcomes(f.source);
        check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::cancelled);
    }
    for(bool npc:{false,true}){
        Fixture f(npc);f.offhand();const auto before=f.health();skills::Slots bar{};bar[0]={33,0};
        check(f.world.configure_adrenaline(f.source,bar,1000));check(f.world.status_mutation(f.source,33,123,12,1000,31000,true));
        check(f.hit(776));check(f.health()==before-76&&!f.world.has_condition(f.target,combat::conditions::Kind::bleeding,f.contact_at));
        check(!f.world.has_condition(f.target,combat::conditions::Kind::deep_wound,f.contact_at)&&f.world.dagger_stage(f.source,f.target,f.contact_at)==2);
        const auto events=f.world.take_attack_outcomes(f.source);check(events.size()==2);
        for(const auto& event:events)check(event.outcome==combat::weapon::AttackOutcome::replaced&&!event.before_damage);
    }
    {
        Fixture f(false);check(f.world.spiteful_spirit(f.target,f.source,*effects::spiteful_spirit(0),20,1000));
        f.offhand();f.world.set_vitals(1,{1000,1000,30,30},100);
        check(f.hit(776));check(f.world.resources(1)->health==995); // one logical Spiteful action, two strike events
        check(f.world.take_attack_outcomes(f.source).size()==2);
    }
    std::cout<<"PASS candidate real dual group and six-family contacts; native/rendered retail evidence pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
