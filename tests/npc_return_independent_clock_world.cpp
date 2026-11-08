#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC independent return clock line "+std::to_string(at.line()));
}
int main()try{
    for(bool elemental:{false,true}){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Old target",1600,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,0,100},100);const auto source=*w.effect_scope(100),target=*w.effect_scope(1);
        check(w.elemental_armor(target,60,60));
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=300;
        p.acquire=2000;p.follow=2500;p.leash=10;p.flight={100,32};p.evidence="synthetic return-home clock independence fixture";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_flare_ai(p,1000));
        const auto returning=[&]{return elemental?w.npc_elemental_ai_snapshot(source)->returning:w.npc_flare_ai_snapshot(source)->returning;};
        w.tick(1000);w.tick(1100);check(returning());
        check(w.settle_resources(target,5000));
        // Home is a known NPC-owned point, independent of the discarded foe's
        // future Health/Energy. Thirty units suffice to return from leash10.
        w.tick(1200);check(!returning());
    }
    std::cout<<"NPC return-home proceeds independently of former-target resource clocks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
