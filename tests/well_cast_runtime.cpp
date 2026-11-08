#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Well cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned skill:{91u,92u,93u,94u})for(unsigned mode=0;mode<4;++mode){
        world::Instance w;
        world::Instance::Npc corpse;
        corpse.id=corpse.definition.id=100;
        corpse.definition.visual_adjustment={0,0,0,100};
        corpse.x=80;
        combat::npcs::Definition enemy;
        enemy.binding.map=146;
        enemy.binding.actor=enemy.binding.definition=100;
        enemy.relation=combat::npcs::Relation::hostile_to_players;
        enemy.spawn_health=combat::Health{100,100};
        enemy.damageable=true;
        enemy.fleshy=true;enemy.fleshy_evidence="test authored";
        corpse.combat_definition=enemy;
        w.configure_npcs({corpse});
        w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
        w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),body=*w.effect_scope(100);
        if(mode==0||mode==2||mode==3)
            check(w.damage_npc(caster,body,{combat::Kind::armor_ignoring,1000,20,194},1248,500).killed);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        std::vector<world::Packet> packets;unsigned claims=0;
        hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){check(cost==(skill==91?5u:skill==94?25u:10u));return w.consume_energy(1,cost,caster,at);};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.target_valid=[](auto id,float,bool self){return id==1&&self;};
        if(mode!=3)hooks.well_corpse=[&](auto owner,const auto& plan,auto attempt,auto at)->skills::casting::EffectCompletion{
            ++claims;check(owner==caster&&plan.skill==skill&&attempt);
            auto result=w.complete_well_cast(owner,plan,attempt,at,252);
            return {result.completed(),std::move(result.visuals)};
        };
        skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;
        skills::Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[skill>=93?5:4]=12;
        const auto admitted=runtime.start(1000,{skill,0,0},bar,policy,ranks,true,hooks);
        check(admitted==(mode==3?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
        if(mode==3){check(w.resources(1)->energy==100&&claims==0);continue;}
        check(w.resources(1)->energy==(skill==91?95u:skill==94?75u:90u));
        const auto finished=skill==94?4000ull:2000ull;
        runtime.update(finished-1,hooks);check(claims==0);
        if(mode==2)runtime.cancel(hooks);
        runtime.update(finished,hooks);
        if(mode!=2)check(claims==1);else check(claims==0);
        const auto field=w.well_field_effect_at(0,80,0,skill>=93?8:7,finished);
        check(field&&field->health_pips==(mode==0?(skill==93?-4:skill==94?0:4):0));
        if(mode==0&&skill==91)check(field->energy_pips==2);
        if(mode==0&&skill==94)check(field->strip_enchantments&&field->block_enchantments);
        const auto primary=skills::particles::positional_effect_2022(0,skill==91?173:skill==92?175:skill==93?176:177,80,0,0);
        check(std::count(packets.begin(),packets.end(),primary)==(mode==0?1:0));
        const auto ground=skills::particles::well_ground_2022(skill,1,80,0,0);
        for(const auto& packet:ground)check(std::count(packets.begin(),packets.end(),packet)==(mode==0?1:0));
        check(std::count(packets.begin(),packets.end(),skills::casting::terminal(58,1))==(mode==0?1:0));
        if(mode==0)check(runtime.start(finished+750,{skill,0,0},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
    }
    for(double roll:{.1,.5}){
        world::Instance w;
        world::Instance::Npc npc;npc.id=npc.definition.id=100;
        npc.definition.visual_adjustment={0,0,0,100};npc.x=80;
        combat::npcs::Definition enemy;enemy.binding.map=146;
        enemy.binding.actor=enemy.binding.definition=100;
        enemy.relation=combat::npcs::Relation::hostile_to_players;
        enemy.spawn_health=combat::Health{100,100};enemy.damageable=true;
        enemy.fleshy=true;enemy.fleshy_evidence="test authored";
        npc.combat_definition=enemy;w.configure_npcs({npc});
        w.join(1,"Low rank",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
        w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),body=*w.effect_scope(100);
        const auto plan=*skills::effects::well_cast_plan(94,4);
        check(w.complete_well_cast(caster,plan,1,600,252,roll).outcome==world::Instance::WellCastOutcome::rejected);
        check(w.damage_npc(caster,body,{combat::Kind::armor_ignoring,1000,20,194},1248,700).killed);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        std::vector<world::Packet> packets;
        hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){check(cost==25);return w.consume_energy(1,cost,caster,at);};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.target_valid=[](auto id,float,bool self){return id==1&&self;};
        hooks.well_corpse=[&](auto owner,const auto& cast,auto attempt,auto at)->skills::casting::EffectCompletion{
            auto result=w.complete_well_cast(owner,cast,attempt,at,252,roll);
            check(result.outcome==(roll<.5?world::Instance::WellCastOutcome::failed_roll:
                                        world::Instance::WellCastOutcome::created));
            return {result.completed(),std::move(result.visuals)};
        };
        skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[94]=1;
        skills::Slots bar{};bar[0]={94,0};attributes::Ranks ranks{};ranks[5]=4;
        check(runtime.start(1000,{94,0,0},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
        check(w.resources(1)->energy==75);
        runtime.update(4000,hooks);
        check(std::count(packets.begin(),packets.end(),skills::casting::terminal(58,1))==1);
        check(std::count(packets.begin(),packets.end(),skills::casting::terminal(59,1))==0);
        check(runtime.start(4750,{94,0,0},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
        const auto visual=skills::particles::positional_effect_2022(0,177,80,0,0);
        check(std::count(packets.begin(),packets.end(),visual)==(roll<.5?0:1));
        const auto field=w.well_field_effect_at(0,80,0,8,4000);
        check(field&&field->block_enchantments==(roll>=.5));
        unsigned offers=0;
        check(!w.claim_well_corpse(caster,plan,2,4001,252,[&](const auto&){++offers;return false;}));
        check(offers==(roll<.5?1u:0u));
    }
    std::cout<<"PASS opt-in paid Well91..94 casts, native ground packets, low-rank completed failure, cancellation and absent capability\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
