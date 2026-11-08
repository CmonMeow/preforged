#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>

using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Feedback line "+std::to_string(at.line()));
}

int main()try{
    using namespace skills;
    for(unsigned rank:{0u,12u,15u,20u}){
        const auto plan=effects::enchantment_removal(1061,rank);
        check(plan&&plan->target_energy_loss==effects::interpolate(4,10,rank));
        check(plan->energy==0&&plan->health==0&&plan->count==1);
    }
    check(!effects::enchantment_removal(1061,21));
    for(bool npc:{false,true})for(bool enchanted:{false,true}){
        world::Instance w;auto out=std::make_shared<world::Outbox>();
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={1000,1000};d.spawn_energy=combat::npcs::Energy{40,100};
            d.energy_evidence="test";d.energy_pips=0;
            d.damageable=true;d.healing_allowed=true;
            d.healing_evidence="test";d.evidence.armor="test";
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Caster",0,0,out,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        if(!npc){
            w.join(2,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{1000,1000,40,100},100);
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        if(enchanted){
            if(npc)check(w.stage_npc_vigorous_spirit(target,*skills::effects::vigorous_spirit(12),100));
            else check(w.status_mutation(target,180,1,12,100,60100,true));
        }
        const auto plan=*effects::enchantment_removal(1061,12);
        auto forged=plan;++forged.target_energy_loss;
        check(!w.enchantment_removal(source,target,forged,20,100));
        const auto before=npc?w.npc_energy_snapshot(target).value().current:w.resources(2)->energy;
        out->drain();
        if(!w.enchantment_removal(source,target,plan,20,100))
            throw std::runtime_error("Feedback removal rejected npc="+std::to_string(npc)+" enchanted="+std::to_string(enchanted));
        const auto after=npc?w.npc_energy_snapshot(target).value().current:w.resources(2)->energy;
        check(after==before-(enchanted?plan.target_energy_loss:0));
        check(w.resources(1)->energy==100);
        if(!npc)check(!w.has_enchantment(target,180,100));
        const auto packets=out->drain();
        const auto body=skills::particles::targeted_body_2022(target.actor,1,1061);
        check(bool(body));
        if(enchanted)check(std::find(packets.begin(),packets.end(),*body)!=packets.end());
    }
    {
        world::Instance w;auto out=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,out,0,{}, {},100,1);
        w.join(2,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.set_vitals(2,{1000,1000,40,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.status_mutation(target,180,1,12,100,60100,true));
        skills::casting::Runtime cast(1);skills::casting::Hooks hooks;
        unsigned completed=0;
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.events=[&](const auto& packets){w.skill_event(1,packets,3000);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){check(cost==10);return w.consume_energy(1,cost,source,at);};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.projectile_target_valid=[&](auto id,float range){return id==2&&range==1248.f;};
        hooks.enchantment_removal=[&](auto victim,const auto& plan,auto at){++completed;return w.enchantment_removal(source,victim,plan,20,at);};
        skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[1061]=1;
        skills::Slots bar{};bar[0]={1061,0};attributes::Ranks ranks{};ranks[3]=12;
        out->drain();
        check(cast.start(1000,{1061,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
        check(w.resources(1)->energy==90);
        cast.update(2999,hooks);check(w.resources(2)->energy==40);
        cast.update(3000,hooks);
        // One Energy regenerates over this cast before the rank-12 loss of 9.
        if(completed!=1||w.resources(2)->energy!=32)
            throw std::runtime_error("Feedback cast completed="+std::to_string(completed)+" target Energy="+std::to_string(w.resources(2)->energy));
        const auto packets=out->drain();
        check(std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1))!=packets.end());
    }
    std::cout<<"PASS Feedback rank, conditional player/NPC Energy loss and paid native cast\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
