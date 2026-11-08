#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC AI visibility references line "+std::to_string(at.line()));
}
std::vector<world::Packet> references(unsigned actor){return {
    agent::facing_2022(actor,1),combat::weapon::timing_2022(actor,1.5f,1),
    combat::weapon::start_2022(actor,1),combat::weapon::stop_2022(actor),
    skills::casting::activation(actor,1,194),
    skills::particles::spell_projectile_launch_2022(actor,100,0,0,1,214),
    combat::detail::effects(actor,0),npc::literal_name_2022(actor,u"Visible name")
};}
int main()try{
    world::Outbox box;box.npc_visibility({100,101},{101});
    const auto hidden=references(100),visible=references(101);
    box.enqueue(visible);check(box.drain()==visible);
    box.enqueue(hidden);const auto leaked=box.drain();
    if(!leaked.empty()){
        std::string opcodes;for(const auto& packet:leaked)opcodes+=" "+std::to_string((packet[0]|unsigned(packet[1])<<8)&0x7fff);
        throw std::runtime_error("hidden NPC references leaked through enqueue; native opcodes:"+opcodes);
    }
    {const auto reservation=box.reserve_publication();check(reservation.has_value());
        check(box.resolve_publication(*reservation,hidden));check(box.drain().empty());}
    {world::Outbox::DeferredQueue sink;{world::Outbox::Capture capture(box,sink);box.enqueue(hidden);}
        check(sink.packets.empty()&&sink.bytes==0);}
    combat::Actor target;target.id=1;target.generation=1;target.health=combat::Health{100,100};target.damageable=true;
    const auto hit=combat::weapon::apply_resolved(target,10,true);check(hit.has_value());
    const auto damage=combat::weapon::packets_2022(100,*hit);
    box.enqueue(damage);
    // Native162 accepts unknown causes. Retain the visible victim's damage
    // notification and Health; only the hidden actor's separate158 is filtered.
    const std::vector<world::Packet> visible_damage{damage[1],damage[2]};
    check(box.drain()==visible_damage);
    {const auto reservation=box.reserve_publication();check(reservation.has_value());
        check(box.resolve_publication(*reservation,damage));
        check(box.drain()==visible_damage);}
    box.npc_visibility({100,101},{100,101});box.enqueue(hidden);check(box.drain()==hidden);
    std::cout<<"NPC AI native references respect centralized visibility\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
