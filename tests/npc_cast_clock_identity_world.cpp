#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC cast clock identity line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,first,second;bool elemental;
    explicit Fixture(bool multi):elemental(multi){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;d.knockdown_allowed=true;d.knockdown_evidence="synthetic source-control clock fixture";
        n.combat_definition=d;w.configure_npcs({n});join(1,100,100);join(2,300,100);
        source=*w.effect_scope(100);first=*w.effect_scope(1);second=*w.effect_scope(2);
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=220;p.acquire=1000;p.follow=1500;p.leash=2000;
        p.flight={1000,32};p.evidence="synthetic NPC cast identity/clock fixture";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_flare_ai(p,1000));
        w.tick(1000);check(cast().has_value());check(selected()==first);
    }
    void join(unsigned id,float x,unsigned now){w.join(id,"Target",x,0,std::make_shared<world::Outbox>(),0,{}, {},now,id);
        w.set_vitals(id,{1000,1000,0,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
    std::optional<combat::npcs::control::Token> cast(){
        if(!elemental)return w.npc_flare_ai_snapshot(source)->cast;
        const auto state=w.npc_elemental_ai_snapshot(source);return state->pending?std::optional(state->pending->token):std::nullopt;
    }
    skills::effects::Scope selected(){return elemental?w.npc_elemental_ai_snapshot(source)->target:w.npc_flare_ai_snapshot(source)->policy.target;}
    std::size_t flights(){return elemental?w.npc_elemental_ai_snapshot(source)->flights:w.npc_flare_ai_snapshot(source)->shots.size();}
};
int main()try{
    for(bool elemental:{false,true}){
        {Fixture f(elemental);const auto old=*f.cast();const auto energy=f.w.npc_energy_snapshot(f.source)->current;
            f.w.leave(1);f.join(1,100,1200);const auto replacement=*f.w.effect_scope(1);check(replacement!=f.first);
            check(f.w.settle_resources(replacement,old.completes+3000));f.w.tick(old.completes);
            check(!f.cast()&&f.flights()==0);check(f.w.npc_energy_snapshot(f.source)->current==energy);
            check(f.w.resources(1)->health==1000);f.w.tick(old.completes+100);
            check(f.cast().has_value()&&f.selected()==f.second);}
        {Fixture f(elemental);const auto old=*f.cast();const auto energy=f.w.npc_energy_snapshot(f.source)->current;
            check(f.w.settle_resources(f.first,old.completes+3000));
            check(f.w.knockdown_npc(f.source,2000,1100)==combat::npcs::control::Knockdown::accepted);
            f.w.tick(1200);check(!f.cast()&&f.flights()==0);check(f.w.npc_energy_snapshot(f.source)->current==energy);
            f.w.tick(1300);check(!f.cast());}
        {Fixture f(elemental);const auto old=*f.cast();const auto future=old.completes+1000;
            check(f.w.settle_resources(f.first,future));f.w.tick(old.completes);
            check(f.cast()==old&&f.flights()==0);check(f.w.resources(1)->health==1000);
            f.w.tick(future);check(!f.cast()&&f.flights()==1);
            f.w.tick(future+200);check(f.w.resources(1)->health<1000);}
    }
    std::cout<<"NPC cast retirement follows source epoch and exact target lifetime clocks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
