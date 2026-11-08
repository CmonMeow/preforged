#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC pursuit facing World line "+std::to_string(at.line()));
}
std::uint32_t word(const world::Packet& p,std::size_t i){return std::uint32_t(p[i])|std::uint32_t(p[i+1])<<8|
    std::uint32_t(p[i+2])<<16|std::uint32_t(p[i+3])<<24;}
float real(const world::Packet& p,std::size_t i){return std::bit_cast<float>(word(p,i));}
int main()try{
    for(bool elemental:{false,true}){
        world::Instance w;auto target_box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.facing=0;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{5,30};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"North target",0,1600,target_box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(100);check(w.elemental_armor(*w.effect_scope(1),60,60));
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=300;p.acquire=2000;p.follow=2500;p.leash=2500;
        p.flight={100,32};p.evidence="synthetic north-facing pursuit fixture; authored speed and fixed target pose";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_flare_ai(p,1000));
        w.tick(1000);target_box->drain();w.tick(1100);
        const auto moved=target_box->drain();const auto facing=std::find_if(moved.begin(),moved.end(),[](const auto& packet){
            return packet.size()==14&&packet[0]==46&&packet[1]==0&&word(packet,2)==100;
        });
        check(facing!=moved.end()&&std::abs(real(*facing,6)-std::acos(-1.f)/2)<.001f);
        w.tick(1200);const auto straight=target_box->drain();
        check(std::none_of(straight.begin(),straight.end(),[](const auto& packet){
            return packet.size()==14&&packet[0]==46&&packet[1]==0&&word(packet,2)==100;
        })); // unchanged heading needs no repeated turn command
        auto observer=std::make_shared<world::Outbox>();w.join(2,"Late observer",5000,5000,observer,0,{}, {},1200,2,false);
        // The session publishes NPC entry separately from player join. Read
        // that authoritative late-observer snapshot, rather than an empty queue.
        const auto packets=w.npc_entry_packets(2);const auto spawn=std::find_if(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>=30&&packet[0]==32&&packet[1]==0&&word(packet,2)==100;
        });
        check(spawn!=packets.end());
        const auto x=real(*spawn,12),y=real(*spawn,16),dx=real(*spawn,22),dy=real(*spawn,26);
        check(std::abs(x)<.01f&&std::abs(y-60)<.01f); //two100ms intervals * authored300 units/sec
        if(std::abs(dx)>.001f||std::abs(dy-1)>.001f)throw std::runtime_error(
            std::string(elemental?"elemental":"Flare")+" moved north to("+std::to_string(x)+","+std::to_string(y)+
            ") but late-observer spawn facing=("+std::to_string(dx)+","+std::to_string(dy)+") expected(0,1)");
        check(w.kill(*w.effect_scope(1),1300));w.tick(1300);observer->drain();w.tick(1400);
        const auto returned=observer->drain();const auto turn=std::find_if(returned.begin(),returned.end(),[](const auto& packet){
            return packet.size()==14&&packet[0]==46&&packet[1]==0&&word(packet,2)==100;
        });
        check(turn!=returned.end()&&std::abs(real(*turn,6)+std::acos(-1.f)/2)<.001f);
    }
    std::cout<<"NPC pursuit late-observer facing passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
