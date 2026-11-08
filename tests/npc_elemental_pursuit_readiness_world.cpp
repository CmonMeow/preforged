#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Elemental pursuit readiness line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,target;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(unsigned pips,bool armor=true,bool ice=false){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding={145,100,100,0,0,0,0,1};
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};
        d.spawn_energy=combat::npcs::Energy{0,40};d.energy_pips=pips;d.damageable=true;n.combat_definition=d;
        w.configure_npcs({n});w.join(1,"Distant foe",1400,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);source=*w.effect_scope(100);target=*w.effect_scope(1);
        if(armor)check(w.elemental_armor(target,0,0));
        combat::ai::FlarePolicy policy;policy.source=source;policy.level=1;policy.speed=100;
        policy.acquire=1500;policy.follow=1800;policy.leash=2000;policy.flight={1000,32};
        policy.evidence="explicit synthetic elemental pursuit policy; no ordinary attack";
        if(ice)check(w.configure_npc_elemental_ai(policy,{{{214,0},*skills::effects::water_projectile(214,0),policy.evidence}},4,100));
        else check(w.configure_npc_elemental_ai(policy,{{{194,0},*skills::effects::elemental(194,0),policy.evidence}},4,100));
        check(w.configure_npc_wander(source,{{{80,0},{0,80}},200,4000,policy.evidence},100));box->drain();
    }
};
int main()try{
    for(unsigned pips:{0u,4u}){
        Fixture f(pips);f.w.tick(100);
        check(f.w.npc_elemental_ai_snapshot(f.source)->engaged);
        const auto initial=f.w.npc_pursuit_movement_snapshot(f.source);
        check(initial&&initial->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
        f.w.tick(1100);const auto moving=f.w.npc_pursuit_movement_snapshot(f.source);
        check(moving&&moving->outcome.travelled>0&&!f.w.npc_elemental_ai_snapshot(f.source)->pending);
        check(f.w.resources(1)->health==1000&&!f.w.npc_ordinary_pending(f.source));
        bool fought=false;
        for(unsigned now=1200;now<=15000;now+=100){
            f.w.tick(now);const auto ai=*f.w.npc_elemental_ai_snapshot(f.source);
            check(!f.w.npc_ordinary_pending(f.source));
            if(!pips)check(!ai.pending&&f.w.npc_energy_snapshot(f.source)->current==0);
            fought=fought||f.w.resources(1)->health<1000;
        }
        check(fought==(pips!=0));
        check(f.w.kill(f.target,15100));bool returning=false,home=false,resumed=false;unsigned home_at=0;
        for(unsigned now=15100;now<=23000;now+=100){
            f.w.tick(now);const auto ai=*f.w.npc_elemental_ai_snapshot(f.source);
            returning=returning||ai.returning;
            if(returning&&!ai.returning&&!ai.engaged&&!home){home=true;home_at=now;}
            const auto move=f.w.npc_pursuit_movement_snapshot(f.source);
            if(home&&now>home_at+200&&move&&move->at==now&&move->outcome.travelled>0)resumed=true;
        }
        check(returning&&home&&resumed);
    }
    // Movement fallback must not convert missing authority into a usable book.
    for(bool ice:{false,true}){
        Fixture f(0,!ice?false:true,ice);
        f.w.tick(100);f.w.tick(1100);
        check(!f.w.npc_pursuit_movement_snapshot(f.source));
        check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.resources(1)->health==1000);
        check(f.w.npc_energy_snapshot(f.source)->current==0);
        if(ice){
            check(f.w.observe_npc_overcast(f.source,0,1100,"explicit test observation"));
            f.w.tick(1200);check(f.w.npc_pursuit_movement_snapshot(f.source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
            f.w.tick(1300);check(f.w.npc_pursuit_movement_snapshot(f.source)->outcome.travelled>0);
        }
    }
    std::cout<<"Known elemental geometry pursues without payment; resources gate combat; return resumes idle\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
