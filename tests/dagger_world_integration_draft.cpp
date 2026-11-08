// UNREGISTERED acceptance fixture for the proposed real World integration.
// Compiles against existing public World APIs; intentionally fails until782/
// 780 canonical plans and actual contact hooks are integrated. No test bridge,
// private-member exposure, alternate damage or cost authority is introduced.
#include "gw_world_simulation.h"
#include "gw_dagger_attack_effects_detached.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("dagger World integration draft line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance world;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,target;
    combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,32,0.};
    Fixture(bool npc,unsigned hp=1000,std::optional<bool> flesh=true){
        world.configure_attack_miss_draws([]{return .5;});
        world.configure_attack_critical_draws([]{return .999999;});
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;
            n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
            d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=true;
            d.physical_armor=60;d.physical_armor_evidence="synthetic dagger recipient";
            d.fleshy=flesh;if(flesh)d.fleshy_evidence="synthetic explicit dagger flesh trait";
            n.combat_definition=d;world.configure_npcs({n});
        }
        world.join(1,"Assassin",0,0,box,0,{}, {},100,1);
        world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);
        if(!npc){world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            world.set_vitals(2,{hp,1000,30,30},100);target=*world.effect_scope(2);
            check(world.physical_armor(target,60));
            check(world.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));
        }else target=*world.effect_scope(100);
        check(world.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{7,0,20}));box->drain();
    }
    combat::weapon::AttackPlan plan(unsigned id,unsigned rank=0){
        const auto adapter=combat::weapon::detached_dagger_attack_plan(id,rank,0);
        check(adapter&&adapter->required_type==32); // catches confirmed8vs32 adapter defect
        // Future integration's explicit World-owner opt-in; normal Runtime
        // attack_skill(id,rank) remains unsupported until paid consumers pass.
        const auto result=combat::weapon::attack_skill(id,rank,0,0,true);check(bool(result));
        check(result->required_type==32); // GWCA item type, NOT catalog mask8.
        check(result->windup_ms==250&&adapter->windup_ms==250); // oldwiki half listed activation rule
        check(result->bonus==adapter->bonus&&result->skill==adapter->skill&&result->rank==adapter->rank);
        return *result;
    }
    bool hit(unsigned id,std::uint64_t at=1000,unsigned rank=0){return world.melee_attack(source,target,weapon,plan(id,rank),at);}
    double health()const{return target.actor==100?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
};
static unsigned word(const world::Packet& packet,unsigned at){
    return unsigned(packet[at])|(unsigned(packet[at+1])<<8)|(unsigned(packet[at+2])<<16)|(unsigned(packet[at+3])<<24);
}
static bool primary_damage(const world::Packet& packet){
    return packet.size()==18&&packet[0]==162&&packet[1]==0&&(word(packet,2)==16||word(packet,2)==17);
}
int main()try{
    for(bool npc:{false,true}){
        Fixture f(npc);check(f.hit(782));check(f.health()==980);
        check(f.world.has_condition(f.target,combat::conditions::Kind::bleeding,1000));
        check(f.world.dagger_stage(f.source,f.target,1000)==1);
        auto packets=f.box->drain();
        const auto stage=std::find(packets.begin(),packets.end(),effects::iron_palm_chain_2022(1,f.target.actor,1));
        const auto damage=std::find_if(packets.begin(),packets.end(),primary_damage);
        check(stage!=packets.end()&&damage!=packets.end()&&stage<damage);
        check(std::count(packets.begin(),packets.end(),*stage)==1);
        const auto events=f.world.take_attack_outcomes(f.source);
        check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::hit&&events[0].before_damage==1000);
        check(f.hit(780,1000));check(f.health()==950);
        check(f.world.dagger_stage(f.source,f.target,1000)==2);
        const auto second=f.world.take_attack_outcomes(f.source);
        check(second.size()==1&&second[0].event>events[0].event);
        check(f.world.dagger_stage(f.source,f.target,16000)==0);
    }
    for(bool npc:{false,true}){
        Fixture missed(npc);check(missed.world.apply_condition(missed.source,missed.source,combat::conditions::Kind::blind,100,5000));
        missed.world.configure_attack_miss_draws([]{return 0.;});check(missed.hit(782));
        check(missed.health()==1000&&!missed.world.has_condition(missed.target,combat::conditions::Kind::bleeding,1000));
        check(missed.world.dagger_stage(missed.source,missed.target,1000)==0);
        const auto events=missed.world.take_attack_outcomes(missed.source);
        check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::miss&&!events[0].before_damage);
        Fixture blocked(npc);blocked.world.configure_attack_block_draws([]{return 0.;});
        if(npc)check(blocked.world.stage_npc_shield_deflection(blocked.target,*effects::shield_deflection(12),100));
        else check(blocked.world.apply_health_regeneration(blocked.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,blocked.target));
        check(blocked.hit(782)&&blocked.health()==1000);
        check(blocked.world.dagger_stage(blocked.source,blocked.target,1000)==0);
        // Existing Iron Palm provides a trusted lead without blocked weapon contact.
        check(blocked.world.iron_palm(blocked.source,blocked.target,*effects::iron_palm(0),20,1000));
        const auto before=blocked.health();check(blocked.hit(780,1000));check(blocked.health()==before-30);
        check(blocked.world.dagger_stage(blocked.source,blocked.target,1000)==2);
    }
    for(bool npc:{false,true}){
        Fixture lethal(npc,10);check(lethal.hit(782));check(lethal.health()==0);
        check(lethal.world.dagger_stage(lethal.source,lethal.target,1000)==0); // private survivor-only policy
        Fixture fragility(npc,21);
        check(fragility.world.degeneration_hex(fragility.source,fragility.target,*effects::degeneration_hex(19,0),100));
        check(fragility.hit(782));check(fragility.health()==0);
        check(fragility.world.dagger_stage(fragility.source,fragility.target,1000)==0);
    }
    {
        Fixture f(false);f.world.set_vitals(1,{10,1000,30,30},100);
        check(f.world.apply_insidious_parasite(f.target,f.source,12,100));
        f.hit(782);check(f.world.resources(1)->health==0&&f.health()==1000);
        check(f.world.dagger_stage(f.source,f.target,1000)==0);
    }
    {
        Fixture f(false);const auto old=f.target;f.world.leave(2);
        f.world.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},200,2);
        f.world.set_vitals(2,{1000,1000,30,30},200);check(*f.world.effect_scope(2)!=old);
        check(!f.world.melee_attack(f.source,old,f.weapon,f.plan(782),1000));
        check(f.world.resources(2)->health==1000&&f.world.dagger_stage(f.source,*f.world.effect_scope(2),1000)==0);
    }
    {
        Fixture f(true,1000,std::nullopt);check(!f.hit(782)&&f.health()==1000);
        check(f.world.take_attack_outcomes(f.source).empty()); // refuse BEFORE release/qualification
        Fixture immune(true,1000,false);check(immune.hit(782)&&immune.health()==980);
        check(!immune.world.has_condition(immune.target,combat::conditions::Kind::bleeding,1000));
        check(immune.world.dagger_stage(immune.source,immune.target,1000)==1);
    }
    {
        Fixture f(true,1000,std::nullopt);
        f.world.join(3,"Parasite caster",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        f.world.set_vitals(3,{1000,1000,30,30},100);f.world.set_vitals(1,{10,1000,30,30},100);
        check(f.world.apply_insidious_parasite(*f.world.effect_scope(3),f.source,12,100));
        check(!f.hit(782));check(f.world.resources(1)->health==10&&f.health()==1000);
        check(f.world.take_attack_outcomes(f.source).empty());
    }
    for(bool npc:{false,true}){
        Fixture f(npc);const auto energy=f.world.resources(1)->energy;
        check(!f.hit(780));check(f.health()==1000&&f.world.resources(1)->energy==energy);
        check(f.world.take_attack_outcomes(f.source).empty());
        check(f.world.iron_palm(f.source,f.target,*effects::iron_palm(0),20,1000));
        const auto before=f.health();check(!f.hit(780,16000));check(f.health()==before);
        check(f.world.take_attack_outcomes(f.source).empty());
    }
    for(bool npc:{false,true})for(unsigned id:{782u,780u}){
        Fixture f(npc,1000,std::nullopt);
        if(id==780)check(f.world.iron_palm(f.source,f.target,*effects::iron_palm(0),20,1000));
        const auto before=f.health();const auto stage=id==780?1u:0u;
        skills::Slots bar{};bar[0]={33,0};check(f.world.configure_adrenaline(f.source,bar,1000));
        check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1000,5000));
        unsigned draws=0;f.world.configure_attack_miss_draws([&]{++draws;return 0.;});
        f.world.configure_attack_block_draws([&]{++draws;return 0.;});
        check(f.world.status_mutation(f.source,33,123,12,1000,31000,true));
        check(f.hit(id));check(f.health()==before-34&&f.world.dagger_stage(f.source,f.target,1000)==stage);
        check(!f.world.has_condition(f.target,combat::conditions::Kind::bleeding,1000)&&draws==0);
        const auto events=f.world.take_attack_outcomes(f.source);
        check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::replaced&&!events[0].before_damage);
    }
    {
        Fixture f(false);skills::Slots bar{};bar[0]={33,0};check(f.world.configure_adrenaline(f.source,bar,100));
        check(f.world.status_mutation(f.source,33,123,12,100,30100,true));
        check(!f.hit(780)&&f.health()==1000); // out-of-sequence attacks do not trigger IW
        check(f.world.take_attack_outcomes(f.source).empty());
    }
    {
        Fixture f(false);check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::weakness,100,5000));
        check(f.world.status_mutation(f.target,299,50,12,100,8100,true));
        check(f.hit(782)&&f.health()==1000); // accepted zero-damage hit still applies lead/Bleeding
        check(f.world.has_condition(f.target,combat::conditions::Kind::bleeding,1000));
        check(f.world.dagger_stage(f.source,f.target,1000)==1);
    }
    {
        Fixture f(true);check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::weakness,100,5000));
        check(f.world.stage_npc_shield_absorption(f.target,12,100));
        const auto power=*combat::weapon::attack_skill(322,0);
        check(f.world.melee_attack(f.source,f.target,f.weapon,power,1000));
        check(f.world.melee_attack(f.source,f.target,f.weapon,power,1000));
        const auto before=f.health();check(f.hit(782)&&f.health()==before);
        check(f.world.has_condition(f.target,combat::conditions::Kind::bleeding,1000));
        check(f.world.dagger_stage(f.source,f.target,1000)==1);
    }
    std::cout<<"PASS actual World dagger contacts, native stage ordering, miss/block, reaction death and scoped lifetime gates\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
