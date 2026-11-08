#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC motion intent line "+std::to_string(at.line()));
}
int main()try{
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding={145,100,100,0,0,0,0,1};
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
    d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;d.damageable=true;n.combat_definition=d;w.configure_npcs({n});
    w.join(1,"Turning target",500,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
    const auto source=*w.effect_scope(100);
    combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=100;p.acquire=1200;p.follow=1500;p.leash=2000;
    p.flight={1000,32};p.evidence="synthetic authored retained-intent test";
    check(w.configure_npc_immediate_ai(p,{{{155,0},*skills::effects::shadow_damage(155,0),p.evidence}},100));
    const auto position=[&]()->std::optional<std::pair<float,float>>{
        std::optional<std::pair<float,float>> result;
        const auto word=[](const world::Packet& packet,unsigned offset){return std::uint32_t(packet[offset])|
            (std::uint32_t(packet[offset+1])<<8)|(std::uint32_t(packet[offset+2])<<16)|(std::uint32_t(packet[offset+3])<<24);};
        for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&word(packet,2)==100)
            result=std::pair{std::bit_cast<float>(word(packet,6)),std::bit_cast<float>(word(packet,10))};
        return result;
    };
    box->drain();w.tick(100);check(!position());
    check(w.npc_pursuit_movement_snapshot(source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
    // The target begins moving north after the NPC selected its eastward
    // route. Its newer pose must never redirect the preceding travel interval.
    w.move(1,500,500,200);box->drain();w.tick(1200);
    const auto east=position();check(east&&std::abs(east->first-110)<.01&&std::abs(east->second)<.01);
    check(w.resources(1)->health==1000&&!w.npc_flare_ai_snapshot(source)->cast);
    w.tick(1300);const auto turn=position();check(turn&&turn->first>east->first&&turn->second>east->second);
    check(std::hypot(turn->first-east->first,turn->second-east->second)<=10.01);
    check(w.npc_energy_snapshot(source)->current==40&&w.resources(1)->health==1000);
    std::cout<<"Retained route precedes newly observed target pose; no retroactive turn or free budget\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
