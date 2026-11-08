#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Disrupting Dagger World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance world;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope caster,target;
    explicit Fixture(float distance=120,bool armor=true){
        world.join(1,"Caster",0,0,box,0,{}, {},100,1);
        world.join(2,"Target",distance,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        world.set_vitals(1,{500,500,100,100},100);
        world.set_vitals(2,{500,500,100,100},100);
        caster=*world.effect_scope(1);target=*world.effect_scope(2);
        check(world.combat_armor(target,armor?std::optional<int>{60}:std::nullopt));
        box->drain();
    }
    bool launch(unsigned rank=12,std::uint64_t now=1100){
        return world.launch_disrupting_dagger(caster,target,
            *skills::effects::disrupting_dagger(rank),20,now);
    }
    unsigned visual_count(){
        const auto expected=*skills::particles::targeted_body_2022(2,1,571);
        const auto packets=box->drain();
        return unsigned(std::count(packets.begin(),packets.end(),expected));
    }
};
int main()try{
    {
        Fixture f;
        check(f.launch()&&f.world.resources(2)->health==500);
        const auto launched=f.box->drain();
        check(launched.size()==1&&launched.front().size()==29&&launched.front()[0]==163&&
              launched.front()[20]==0x75&&launched.front()[21]==4);
        f.world.tick(1150);check(f.world.resources(2)->health==500&&f.visual_count()==0);
        f.world.tick(1300);check(f.world.resources(2)->health<500&&f.visual_count()==1);
        const auto health=f.world.resources(2)->health;
        f.world.tick(2000);check(f.world.resources(2)->health==health&&f.visual_count()==0);
    }
    {
        Fixture f;
        std::vector<world::Packet> launch_visuals;
        check(f.world.launch_disrupting_dagger(f.caster,f.target,
            *skills::effects::disrupting_dagger(12),20,1100,&launch_visuals));
        check(launch_visuals.size()==1&&launch_visuals.front()[0]==163&&f.box->drain().empty());
        f.world.tick(1300);check(f.visual_count()==1);
    }
    {
        Fixture f;
        check(f.world.cast_activity(f.target,23,1000,4000));
        const auto before=*f.world.control_epoch(f.target);
        check(f.launch());f.world.tick(1300);
        check(f.world.resources(2)->health<500&&f.visual_count()==1);
        check(!f.world.casting_skill(f.target,1300)&&*f.world.control_epoch(f.target)!=before);
    }
    {
        Fixture f;
        check(f.world.cast_activity(f.target,23,1000,1150));
        const auto before=*f.world.control_epoch(f.target);
        check(f.launch());f.world.tick(1300);
        check(f.world.resources(2)->health<500&&*f.world.control_epoch(f.target)==before);
    }
    {
        Fixture f(700);check(!f.launch());f.world.tick(3000);
        check(f.world.resources(2)->health==500&&f.visual_count()==0);
    }
    {
        Fixture f(120,false);check(!f.launch());f.world.tick(1300);
        check(f.world.resources(2)->health==500&&f.visual_count()==0);
    }
    {
        Fixture f;check(f.launch());f.world.leave(1);f.world.tick(1300);
        check(f.world.resources(2)->health==500&&f.visual_count()==0);
    }
    {
        Fixture f;check(f.launch());f.world.leave(2);
        f.world.join(2,"Replacement",120,0,std::make_shared<world::Outbox>(),0,{}, {},1200,2);
        f.world.set_vitals(2,{500,500,100,100},100);
        f.world.tick(3000);check(f.world.resources(2)->health==500&&f.visual_count()==0);
    }
    for(unsigned mode=0;mode<5;++mode){
        Fixture f;
        skills::casting::Hooks hooks;
        unsigned payments=0,commits=0;
        bool stale=false;
        hooks.resources=[&](auto actor){return f.world.resources(actor);};
        hooks.events=[&](const auto& packets){f.world.skill_event(1,packets,1250);};
        hooks.consume_energy_at=[&](auto amount,auto at){++payments;
            check(amount==5);return f.world.consume_energy(1,amount,f.caster,at);};
        hooks.target_scope=[&](auto actor){return stale&&actor==2?
            std::optional<skills::effects::Scope>{}:f.world.effect_scope(actor);};
        hooks.projectile_target_valid=[&](auto actor,float range){return actor==2&&range==624.f;};
        if(mode!=4)hooks.disrupting_dagger=[&](auto target,const auto& plan,auto at)->skills::casting::EffectCompletion{
            ++commits;std::vector<world::Packet> visuals;
            const bool ok=f.world.launch_disrupting_dagger(f.caster,target,plan,20,at,&visuals);
            return {ok,std::move(visuals)};
        };
        skills::Slots bar{};bar[0]={571,0};skills::Policy policy;
        policy.primary=7;policy.campaigns=255;policy.learned[571]=1;
        attributes::Ranks ranks{};ranks[30]=12;
        skills::casting::Runtime cast(1);
        check(cast.start(1000,{571,0,2},bar,policy,ranks,true,hooks)==
            (mode==4?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
        check(payments==unsigned(mode!=4));
        if(mode==4)continue;
        check(f.world.resources(1)->energy==95);
        if(mode==1)cast.cancel(hooks);
        if(mode==2)cast.interrupt(1100,hooks);
        if(mode==3)stale=true;
        cast.update(1250,hooks);
        check(commits==unsigned(mode==0));
        const auto packets=f.box->drain();
        const auto launch=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==29&&p[0]==163;});
        check((launch!=packets.end())==(mode==0));
        if(mode==0){
            const auto finish=std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1));
            const auto recharge=std::find(packets.begin(),packets.end(),skills::casting::slot_event(228,1,{571,0},10));
            check(finish<launch&&launch<recharge);
            check(cast.start(1251,{571,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::busy);
            f.world.tick(1400);check(f.world.resources(2)->health<500&&f.visual_count()==1);
            check(cast.start(2250,{571,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
            check(cast.start(11250,{571,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
            cast.cancel(hooks);
        }else{f.world.tick(1400);check(f.world.resources(2)->health==500);}
    }
    {
        world::Instance world;
        world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.definition.level=20;npc.x=120;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;
        definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
        definition.binding.level=npc.definition.level;
        definition.relation=combat::npcs::Relation::hostile_to_players;
        definition.spawn_health=combat::Health{500,500};definition.damageable=true;
        definition.spawn_energy=combat::npcs::Energy{20,20};definition.energy_pips=0;
        definition.earth_armor=60;definition.evidence.armor="reviewed test";
        npc.combat_definition=definition;world.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();
        world.join(1,"Caster",0,0,box,0,{}, {},100,1);
        world.join(2,"NPC target",140,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        world.set_vitals(1,{500,500,100,100},100);
        world.set_vitals(2,{500,500,100,100},100);
        const auto caster=*world.effect_scope(1),target=*world.effect_scope(100);
        const auto victim=*world.effect_scope(2);
        check(world.elemental_armor(victim,60,60));
        check(world.configure_npc_spellbook(target,{{{1881,0},1248,"DoA cast fixture"}}));
        const auto spell=world.begin_npc_bonds_of_torment(target,victim,1000);
        check(bool(spell));
        box->drain();
        check(world.launch_disrupting_dagger(caster,target,*skills::effects::disrupting_dagger(12),20,1100));
        world.tick(1150);check(world.npc_combat_snapshot(target)->health->current==500);
        world.tick(1300);check(world.npc_combat_snapshot(target)->health->current<500);
        world.tick(2500);check(!world.complete_npc_bonds_of_torment(*spell,2500));
        const auto body=*skills::particles::targeted_body_2022(100,1,571);
        const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),body)==1);
    }
    std::cout<<"PASS delayed player/NPC impact, player interrupt at impact, idle damage, armor/range/retirement\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
