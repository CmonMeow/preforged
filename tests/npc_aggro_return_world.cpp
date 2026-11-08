#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC aggro return World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,first,second;bool elemental;
    Fixture(bool use_elemental,bool leash_case=false):elemental(use_elemental){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{leash_case?50u:0u,50};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
        join(1,leash_case?1600:100);join(2,leash_case?1800:500);
        source=*w.effect_scope(100);first=*w.effect_scope(1);second=*w.effect_scope(2);
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=leash_case?300:1;
        p.acquire=leash_case?2000:1000;p.follow=leash_case?2500:2000;p.leash=leash_case?10:3000;
        p.flight={100,32};p.evidence="synthetic aggro deferral/return fixture; authored resources and radii";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_flare_ai(p,1000));
    }
    void join(unsigned id,float x){w.join(id,"Target",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
        w.set_vitals(id,{1000,1000,0,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
    skills::effects::Scope selected(){return elemental?w.npc_elemental_ai_snapshot(source)->target:w.npc_flare_ai_snapshot(source)->policy.target;}
    bool engaged(){return elemental?w.npc_elemental_ai_snapshot(source)->engaged:w.npc_flare_ai_snapshot(source)->engaged;}
    bool returning(){return elemental?w.npc_elemental_ai_snapshot(source)->returning:w.npc_flare_ai_snapshot(source)->returning;}
    std::pair<bool,bool> state(){return {engaged(),returning()};}
};
int main()try{
    for(bool elemental:{false,true}){
        {Fixture f(elemental);f.w.tick(1000);check(f.engaged()&&f.selected()==f.first);
            check(f.w.settle_resources(f.second,4000));check(f.w.kill(f.first,1100));f.w.tick(1100);
            // B was acquired at1000, but its authoritative resource clock is
            // ahead. Do not fabricate B's earlier state or forget its memory.
            check(!f.engaged()&&!f.returning());f.w.tick(1200);check(!f.engaged()&&!f.returning());
            f.w.tick(4000);check(f.engaged()&&!f.returning()&&f.selected()==f.second);}
        {Fixture f(elemental);f.w.tick(1000);check(f.w.settle_resources(f.second,4000));
            check(f.w.kill(f.first,1100));f.w.tick(1100);check(!f.engaged()&&!f.returning());
            f.w.leave(2);f.w.tick(1200);check(!f.engaged()&&f.returning());
            f.w.tick(1300);check(!f.engaged()&&!f.returning());
            f.w.tick(4000);check(!f.engaged()&&!f.returning());}
        {Fixture f(elemental,true);f.w.tick(1000);check(f.engaged()&&f.selected()==f.first);
            f.w.tick(1100); //30 requested units stop at the authored leash10
            f.w.move(2,2200,0,1100);f.w.tick(1200);
            check(!f.engaged()&&!f.returning()); // same tick reaches home again
            check(f.w.kill(f.first,1250));f.w.tick(3000);
            check(std::abs(f.w.position(2,3000).first-2200)<.01f);
            // B was acquired at1800 before the leash return. Its old memory
            // cannot make B eligible at2200 after this complete return-home.
            check(!f.engaged()&&!f.returning());
            check(f.w.npc_energy_snapshot(f.source)->current==50);}
    }
    std::cout<<"NPC unknown-position deferral and return-home memory passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
