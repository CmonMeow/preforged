#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC Energy transfer line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,npc,player;
    std::vector<world::Instance::Npc> population;
    Fixture(unsigned energy=20,unsigned owner_energy=10,unsigned mode=0,unsigned health=500,unsigned pips=0){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=mode==3?2000:1000;n.plane=mode==4?1:0;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=mode==5?combat::npcs::Relation::neutral:combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{double(health),500};d.damageable=d.conditions_allowed=mode!=5;
        if(mode!=1){d.spawn_energy=combat::npcs::Energy{energy,20};d.energy_evidence="test";}
        if(mode!=1&&mode!=2){d.energy_pips=pips;d.energy_pips_evidence="test";}
        n.combat_definition=d;population.push_back(n);w.configure_npcs(population);
        w.join(1,"Mesmer",0,0,box,0,{},0,100,1);w.set_vitals(1,{500,500,owner_energy,100},100);
        check(w.energy_capacity(1,5,0,100,{80,-2}));
        w.join(2,"Foe",1000,0,std::make_shared<world::Outbox>(),0,{},0,100,2);w.set_vitals(2,{health,500,energy,20},100);
        source=*w.effect_scope(1);npc=*w.effect_scope(100);player=*w.effect_scope(2);box->drain();
    }
};
int main()try{
    for(unsigned id:{79u,80u})for(unsigned rank=0;rank<=20;++rank)for(unsigned energy:{0u,1u,3u,20u})for(unsigned owner:{0u,10u,99u}){
        Fixture f(energy,owner);const auto plan=*effects::energy_drain(id,rank);const auto loss=std::min(energy,plan.amount);
        std::vector<world::Packet> visuals;check(f.w.energy_drain(f.source,f.npc,plan,20,100,&visuals));
        check(f.w.npc_energy_snapshot(f.npc)->current==energy-loss);
        check(f.w.resources(1)->energy==std::min(100u,owner+loss*plan.gain_multiplier));
        check(f.w.npc_combat_snapshot(f.npc)->health->current==500&&f.w.resources(1)->health==500);
        const auto particle=*particles::targeted_body_2022(100,1,id);check(visuals==std::vector<world::Packet>{particle});
        const auto sent=f.box->drain();check(std::count(sent.begin(),sent.end(),particle)==0);
    }
    for(unsigned id:{79u,80u}){
        Fixture f;std::vector<world::Packet> visuals;check(f.w.energy_drain(f.source,f.player,*effects::energy_drain(id,12),20,100,&visuals));
        check(visuals==std::vector<world::Packet>{*particles::targeted_body_2022(2,1,id)});
        const auto plan=*effects::energy_drain(id,12);check(f.w.resources(2)->energy==20-plan.amount&&f.w.resources(1)->energy==10+plan.amount*plan.gain_multiplier);
    }
    for(unsigned id:{79u,80u})for(unsigned mode=1;mode<=8;++mode){
        Fixture f(20,10,mode<=5?mode:0);auto source=f.source,target=f.npc;auto plan=*effects::energy_drain(id,12);
        if(mode==6)++target.generation;if(mode==7)++source.generation;if(mode==8)++plan.gain_multiplier;
        std::vector<world::Packet> visuals;check(!f.w.energy_drain(source,target,plan,20,100,&visuals));
        check(f.w.resources(1)->energy==10&&visuals.empty());if(mode!=1)check(f.w.npc_energy_snapshot(f.npc)->current==20);
    }
    for(unsigned id:{79u,80u})for(bool lethal:{false,true}){
        Fixture f(3,10,0,lethal?50:500);check(f.w.apply_mind_wrack(f.source,f.npc,12,true,100));
        check(f.w.energy_drain(f.source,f.npc,*effects::energy_drain(id,12),20,100));
        check(f.w.npc_energy_snapshot(f.npc)->current==0&&f.w.resources(1)->energy==10+3*(id==79?3:2));
        check(f.w.npc_combat_snapshot(f.npc)->health->current==(lethal?0:417));
        const auto statuses=f.w.status_snapshot(f.npc,100);check(bool(statuses));
        check(std::none_of(statuses->begin(),statuses->end(),[](const auto& e){return e.skill==2734;}));
        if(!lethal){check(f.w.energy_drain(f.source,f.npc,*effects::energy_drain(id,12),20,100));check(f.w.npc_combat_snapshot(f.npc)->health->current==417);}
    }
    // Integer-authorized transfer preserves a positive fractional remainder.
    {Fixture f(8,10,0,500,1);check(f.w.energy_drain(f.source,f.npc,*effects::energy_drain(80,0),20,100));
        check(f.w.apply_mind_wrack(f.source,f.npc,12,true,100));
        check(f.w.energy_drain(f.source,f.npc,*effects::energy_drain(80,0),20,1100));
        check(f.w.npc_energy_snapshot(f.npc)->current==0&&*f.w.npc_precise_energy_snapshot(f.npc)>0);
        check(f.w.resources(1)->energy==26&&f.w.npc_combat_snapshot(f.npc)->health->current==500);
        const auto statuses=f.w.status_snapshot(f.npc,1100);check(bool(statuses));
        check(std::any_of(statuses->begin(),statuses->end(),[](const auto& e){return e.skill==2734;}));}
    // Gains must respect the caster's active Overcast ceiling.
    {Fixture f(20,99);check(f.w.activate_overcast_energy(f.source,219,0,1,100));
        check(f.w.resources(1)->max_energy==100);
        check(f.w.energy_drain(f.source,f.npc,*effects::energy_drain(79,12),20,100));
        check(f.w.resources(1)->energy==95);}
    for(unsigned id:{79u,80u})for(unsigned mode=0;mode<6;++mode){
        Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,payments=0;std::vector<world::Packet> events;
        h.resources=[&](auto actor){return f.w.resources(actor);};h.events=[&](const auto& p){events.insert(events.end(),p.begin(),p.end());};
        h.consume_energy_at=[&](unsigned cost,auto at){++payments;check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
        h.target_scope=[&](auto actor){return f.w.effect_scope(actor);};h.projectile_target_valid=[](auto actor,float range){return actor==100&&range==1248;};
        if(mode!=3)h.energy_drain=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
            ++calls;std::vector<world::Packet> visuals;const bool okay=f.w.energy_drain(f.source,target,plan,20,at,&visuals);return {okay,std::move(visuals)};};
        Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[id]=1;Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};ranks[3]=12;
        const auto end=id==79?1100u:2100u;
        check(runtime.start(100,{id,0,100},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(500,h);if(mode==4)h.energy_drain={};if(mode==5)h.target_scope=[&](auto actor){auto scope=f.w.effect_scope(actor);if(scope&&actor==100)++scope->generation;return scope;};
        runtime.update(end-1,h);check(!calls);runtime.update(end,h);
        const auto current=*f.w.effect_scope(100);const auto plan=*effects::energy_drain(id,12);
        check(calls==unsigned(mode==0)&&payments==unsigned(mode!=3));
        check(f.w.npc_energy_snapshot(current)->current==(mode==0?20-plan.amount:20));
        check(f.w.resources(1)->energy==(mode==0?5+plan.amount*plan.gain_multiplier:mode==3?10:5));
        if(mode==0){const auto finish=casting::generic(58,1,0),effect=*particles::targeted_body_2022(100,1,id);
            std::vector<world::Packet> actual;for(const auto& packet:events)if(packet==finish||packet==effect)actual.push_back(packet);
            check(actual==std::vector<world::Packet>{finish,effect});runtime.update(end,h);check(calls==1&&payments==1);}
    }
    std::cout<<"NPC Energy Drain/Tap bounded transfers,precision,reactions,visuals and cast controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
