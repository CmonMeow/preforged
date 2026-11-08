// Authored patrol stopping publication; no retail timing inference.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC patrol stop publication line "+std::to_string(at.line()));
}
int main()try{
    for(bool elemental:{false,true})for(bool dense:{false,true})for(bool timeout:{false,true}){
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
        check(w.configure_npc_wander(source,{{{80,0},{0,80}},200,timeout?500u:5000u,p.evidence},100));
        float x=0,y=0;unsigned stops=0;
        const auto drain=[&]{
            const auto word=[](const world::Packet& packet,unsigned at){return std::uint32_t(packet[at])|
                (std::uint32_t(packet[at+1])<<8)|(std::uint32_t(packet[at+2])<<16)|(std::uint32_t(packet[at+3])<<24);};
            bool moved=false;
            for(const auto& packet:box->drain()){
                if(packet.size()==6&&packet[0]==40&&!packet[1]&&word(packet,2)==100)++stops;
                if(packet.size()==16&&packet[0]==44&&!packet[1]&&word(packet,2)==100){
                    const float nx=std::bit_cast<float>(word(packet,6)),ny=std::bit_cast<float>(word(packet,10));
                    moved|=std::hypot(nx-x,ny-y)>.001;x=nx;y=ny;
                }
            }
            return moved;
        };
        box->drain();w.tick(100);check(!drain());w.tick(300);check(!drain());
        const unsigned end=timeout?900:1200;
        if(dense)for(unsigned at=400;at<end;at+=100){w.tick(at);drain();}
        w.tick(end);drain();
        check(std::abs(x-(timeout?50.f:79.f))<.02&&std::abs(y)<.01);
        if(stops!=1)std::cerr<<"elemental="<<elemental<<" dense="<<dense<<" timeout="<<timeout<<" stops="<<stops<<'\n';
        check(stops==1);
        w.tick(end+100);check(!drain());check(stops==1);
        bool resumed=false;
        for(unsigned at=end+200;at<=end+500;at+=100){w.tick(at);resumed=drain()||resumed;}
        check(resumed&&y>0);check(w.resources(1)->health==1000);
        check(elemental?!w.npc_elemental_ai_snapshot(source)->engaged:!w.npc_flare_ai_snapshot(source)->engaged);
        check(!w.npc_ordinary_pending(source));
    }
    std::cout<<"Ordinary/elemental patrol arrival and deadline publish one stop across coarse/fine polls then resume\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
