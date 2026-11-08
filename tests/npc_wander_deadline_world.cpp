// Detached until parent registration; exercises authored patrol timing only.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC wander deadline line "+std::to_string(at.line()));
}
int main()try{
    for(bool elemental:{false,true})for(bool dense:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding={145,100,100,0,0,0,0,1};
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        if(elemental){d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;}
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Distant observer",4000,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(100);
        combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=100;p.acquire=200;p.follow=1500;p.leash=2000;
        p.flight={1000,32};p.evidence="synthetic authored idle leg deadline";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,0),p.evidence}},4,100));
        else check(w.configure_npc_hostile_ordinary(p,{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,250,1000,128,{1000,32},p.evidence},100));
        check(w.configure_npc_wander(source,{{{80,0},{0,80}},200,500,p.evidence},100));
        float x=0,y=0;
        const auto drain=[&]{
            const auto word=[](const world::Packet& packet,unsigned at){return std::uint32_t(packet[at])|
                (std::uint32_t(packet[at+1])<<8)|(std::uint32_t(packet[at+2])<<16)|(std::uint32_t(packet[at+3])<<24);};
            bool moved=false;
            for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&word(packet,2)==100){
                x=std::bit_cast<float>(word(packet,6));y=std::bit_cast<float>(word(packet,10));moved=true;
            }
            return moved;
        };
        box->drain();w.tick(100);check(!drain());w.tick(300);check(!drain());
        // Intent begins300 and expires800. A900 poll may consume only500ms,
        // never the extra100ms spent inside the authored idle pause.
        if(dense)for(unsigned at=400;at<=800;at+=100){w.tick(at);drain();}
        w.tick(900);drain();check(std::abs(x-50)<.01&&std::abs(y)<.01);
        w.tick(1000);check(!drain());check(std::abs(x-50)<.01&&std::abs(y)<.01);
        bool resumed=false;
        for(unsigned at=1100;at<=1500;at+=100){w.tick(at);resumed=drain()||resumed;}
        check(resumed&&y>0);check(w.resources(1)->health==1000);
        check(elemental?!w.npc_elemental_ai_snapshot(source)->engaged:!w.npc_flare_ai_snapshot(source)->engaged);
        check(!w.npc_ordinary_pending(source));
    }
    std::cout<<"Ordinary/elemental retained idle legs stop at authored deadline across coarse/fine polls and resume\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
