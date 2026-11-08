#include "gw_world_simulation.h"
#include <iostream>
int main()try{
    using namespace fw::gw::world;
    Instance world;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();
    world.join(1,"Follower",0,0,a);world.join(2,"Leader",440,0,b);a->drain();b->drain();
    if(world.follow(1,1,1000)||world.follow(1,99,1000)||world.follow(99,2,1000))throw std::runtime_error("invalid follow accepted");
    if(!world.follow(1,2,1000))throw std::runtime_error("valid follow rejected");
    a->drain();b->drain();
    if(!world.call_following(1,2)||a->drain()!=fw::gw::chat::local(1,11,u"I'm following Leader.")||!b->drain().empty())
        throw std::runtime_error("following callout must stay in caller's party");
    if(world.call_following(1,1)||world.call_following(2,1)||world.call_following(99,2)||world.call_following(1,99))
        throw std::runtime_error("invented following callout");
    for(std::uint64_t now=1100;now<=4000;now+=100){world.tick(now);a->drain();b->drain();}
    const auto stopped=world.position(1,4000);
    if(stopped.first<340||stopped.first>440||world.position(1,5000)!=stopped)throw std::runtime_error("follow failed to stop near stationary target");
    world.move(2,880,0,4100);
    for(std::uint64_t now=4200;now<=8500;now+=100){world.tick(now);a->drain();b->drain();}
    if(world.position(1,8500).first<780)throw std::runtime_error("follow failed to resume when target moved");
    world.steer(1,0,0,8500);const auto manual=world.position(1,8500);
    if(world.call_following(1,2))throw std::runtime_error("cancelled follow was announced");
    world.move(2,1320,0,8600);
    for(std::uint64_t now=8700;now<=12000;now+=100){world.tick(now);a->drain();b->drain();}
    if(world.position(1,12000)!=manual)throw std::runtime_error("manual movement did not cancel follow");
    if(!world.follow(1,2,12100))throw std::runtime_error("follow restart failed");
    world.tick(12200);a->drain();b->drain();
    world.leave(2);const auto departed=world.position(1,12200);world.tick(13000);
    if(world.position(1,13000)!=departed)throw std::runtime_error("departed target still followed");
    // Reusing the departed agent ID must not silently retarget the old follow.
    world.join(2,"Replacement",3000,0,b);world.tick(14000);
    if(world.call_following(1,2))throw std::runtime_error("reused agent identity was announced as followed");
    if(world.position(1,14000)!=departed)throw std::runtime_error("follow attached to reused identity");
    if(!world.follow(1,2,14100))throw std::runtime_error("follow replacement request failed");
    world.move(1,departed.first,departed.second+22,14200);
    for(std::uint64_t now=14300;now<=15000;now+=100){world.tick(now);a->drain();b->drain();}
    const auto clicked=world.position(1,15000);
    world.tick(16000);
    if(world.position(1,16000)!=clicked)throw std::runtime_error("ground click did not cancel follow");
    if(!world.follow(1,2,16100)||!world.chat(1,u"/stuck",16150))throw std::runtime_error("stuck test setup failed");
    const auto stuck=world.position(1,16150);world.tick(17000);
    if(world.position(1,17000)!=stuck)throw std::runtime_error("stuck did not cancel follow");
    Instance unicode;auto first=std::make_shared<Outbox>(),second=std::make_shared<Outbox>();
    unicode.join(1,"Caller",0,0,first);unicode.join(2,"\xc3\x89\xe6\x9e\x97",440,0,second);
    unicode.follow(1,2,1000);first->drain();second->drain();
    if(!unicode.call_following(1,2)||first->drain()!=fw::gw::chat::local(1,11,u"I'm following \u00c9\u6797.")||!second->drain().empty())
        throw std::runtime_error("following callout corrupted Unicode character name");
    std::cout<<"PASS follow pursuit, arrival, manual cancellation and target departure\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
