#include "gw_ear_bite.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Ear Bite policy line "+std::to_string(at.line()));
}
int main()try{
    using namespace skills::effects;
    for(unsigned rank=0;rank<=10;++rank){
        const auto p=ear_bite(rank);
        check(p&&p->damage==50+2*rank&&p->bleeding_ms==(15+rank)*1000&&p->range==144.f);
    }
    check(!ear_bite(11));
    combat::Actor foe;foe.id=2;foe.generation=1;foe.health=combat::Health{1000,1000};foe.damageable=true;
    const auto p=*ear_bite(10);
    check(!ear_bite_target(p,foe,20).accepted);
    foe.physical_armor=60;foe.earth_armor=100;
    const auto hit=ear_bite_target(p,foe,20);
    check(hit.accepted&&hit.hit&&hit.hit->kind==combat::Kind::piercing&&hit.hit->damage==70);
    check(!combat::damage_packets_2022(1,*hit.hit).empty());
    auto forged=p;++forged.damage;check(!ear_bite_target(forged,foe,20).accepted);
    std::cout<<"Ear Bite detached policy passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
