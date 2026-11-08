#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC pursuit leash World line "+std::to_string(at.line()));
}
std::uint32_t word(const world::Packet& p,std::size_t i){return std::uint32_t(p[i])|std::uint32_t(p[i+1])<<8|
    std::uint32_t(p[i+2])<<16|std::uint32_t(p[i+3])<<24;}
float real(const world::Packet& p,std::size_t i){return std::bit_cast<float>(word(p,i));}
int main()try{
    for(bool elemental:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{5,30};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Distant target",1600,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
        const auto source=*w.effect_scope(100);check(w.elemental_armor(*w.effect_scope(1),60,60));
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=300;
        p.acquire=2000;p.follow=2500;p.leash=100;p.flight={100,32};
        p.evidence="synthetic authored home-leash fixture; coarse tick enforcement, not retail radius";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_flare_ai(p,1000));
        w.tick(1000);box->drain();w.tick(3000); //600 units requested, home leash100
        const auto moved=box->drain();bool position=false;
        for(const auto& packet:moved)if(packet.size()==16&&packet[0]==44&&packet[1]==0&&word(packet,2)==100){
            check(std::hypot(real(packet,6),real(packet,10))<=100.001f);position=true;
        }
        check(position);
        if(elemental){const auto state=w.npc_elemental_ai_snapshot(source);check(state&&!state->engaged&&state->returning);}
        else{const auto state=w.npc_flare_ai_snapshot(source);check(state&&!state->engaged&&state->returning);}
        // Return is exempt from pursuit clipping and reaches the home stop.
        w.tick(4000);
        if(elemental){const auto state=w.npc_elemental_ai_snapshot(source);check(state&&!state->engaged&&!state->returning);}
        else{const auto state=w.npc_flare_ai_snapshot(source);check(state&&!state->engaged&&!state->returning);}
    }
    std::cout<<"NPC pursuit leash World passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
