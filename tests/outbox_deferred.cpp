#include "gw_world_simulation.h"
#include <thread>
#include <iostream>
#include <source_location>
using namespace fw::gw::world;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Deferred outbox line "+std::to_string(at.line()));
}
int main()try{
    Outbox a,b;Outbox::DeferredQueue first,second;
    const Packet old{1,0},one{2,0},two{3,0},other_thread{4,0};
    a.enqueue({old});
    {Outbox::Capture ca(a,first);Outbox::Capture cb(b,second);
     a.enqueue({one});b.enqueue({two});
     std::thread producer([&]{a.enqueue({other_thread});});producer.join();
     check(a.drain()==std::vector<Packet>({old,other_thread}));check(b.drain().empty());
     bool rejected=false;try{Outbox::Capture nested(a,second);}catch(const std::logic_error&){rejected=true;}
     check(rejected);a.enqueue({two});}
    check(first.packets==std::vector<Packet>({one,two})&&second.packets==std::vector<Packet>({two}));
    a.enqueue({old});a.enqueue(std::move(first.packets));b.enqueue(std::move(second.packets));
    check(a.drain()==std::vector<Packet>({old,one,two}));check(b.drain()==std::vector<Packet>({two}));
    // Existing NPC visibility rules apply during capture too.
    Outbox hidden;hidden.npc_visibility({100},{});Outbox::DeferredQueue filtered;
    {Outbox::Capture capture(hidden,filtered);hidden.enqueue({Packet{65,0,100,0,0,0},one});}
    check(filtered.packets==std::vector<Packet>({one}));
    Outbox bounded;Outbox::DeferredQueue full;
    {Outbox::Capture capture(bounded,full);bounded.enqueue({Packet(65536,0),one});}
    check(full.bytes==65536&&full.packets.size()==1);
    bool overflow=false;try{bounded.drain();}catch(const std::runtime_error&){overflow=true;}check(overflow);
    Outbox ordered;ordered.enqueue({old});const auto barrier=ordered.reserve_publication();check(bool(barrier));
    std::thread later([&]{ordered.enqueue({other_thread});});later.join();
    check(ordered.drain()==std::vector<Packet>{old}&&ordered.drain().empty());
    check(!ordered.resolve_publication(*barrier+1,{two}));
    check(ordered.resolve_publication(*barrier,{one,two})&&!ordered.resolve_publication(*barrier,{one}));
    check(ordered.drain()==std::vector<Packet>({one,two,other_thread}));
    // Reservations inside an owned-effect capture preserve both sides of it.
    Outbox captured;Outbox::DeferredQueue retained;std::optional<std::uint64_t> pending;
    {Outbox::Capture capture(captured,retained);captured.enqueue({old});pending=captured.reserve_publication();
     check(pending&&captured.drain()==std::vector<Packet>{old});captured.enqueue({two});}
    captured.enqueue(std::move(retained.packets));check(captured.drain().empty());
    check(captured.resolve_publication(*pending,{one})&&captured.drain()==std::vector<Packet>({one,two}));
    const auto first_barrier=captured.reserve_publication(),second_barrier=captured.reserve_publication();
    check(first_barrier&&second_barrier&&captured.resolve_publication(*second_barrier,{two})&&captured.drain().empty());
    check(captured.resolve_publication(*first_barrier,{one})&&captured.drain()==std::vector<Packet>({one,two}));
    const auto discarded=captured.reserve_publication();captured.enqueue({two});
    check(discarded&&captured.resolve_publication(*discarded,{})&&captured.drain()==std::vector<Packet>{two});
    Outbox reservation_limit;std::vector<std::uint64_t> reservations;
    for(unsigned i=0;i<128;++i){const auto token=reservation_limit.reserve_publication();check(bool(token));reservations.push_back(*token);}
    check(!reservation_limit.reserve_publication());
    for(const auto token:reservations)check(reservation_limit.resolve_publication(token,{}));
    check(bool(reservation_limit.reserve_publication()));
    Outbox publication_limit;const auto held=publication_limit.reserve_publication();check(bool(held));
    publication_limit.enqueue({Packet(65536,0)});check(!publication_limit.resolve_publication(*held,{one}));
    overflow=false;try{publication_limit.drain();}catch(const std::runtime_error&){overflow=true;}check(overflow);
    std::cout<<"PASS outbound capture:recipient isolation,thread isolation,prefix preservation,nesting guard,visibility and bounded queues\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
