#include "gw_energy_blast.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Energy Blast policy line "+std::to_string(at.line()));
}
int main()try{
    using namespace skills::effects;
    for(unsigned rank=0;rank<=20;++rank){
        const auto p=energy_blast(rank);check(p&&p->maximum_damage==130&&p->range==1248.f);
        check(p->damage_per_energy==(rank>=8?2u:1u));
    }
    check(!energy_blast(21));
    combat::Actor foe;foe.id=2;foe.generation=1;foe.health=combat::Health{1000,1000};foe.damageable=true;
    const auto p=*energy_blast(12);
    check(energy_blast_target(p,foe,0,20).accepted&&!energy_blast_target(p,foe,0,20).hit);
    const auto low=energy_blast_target(p,foe,20,20);check(low.accepted&&low.hit&&low.hit->applied==40);
    const auto capped=energy_blast_target(p,foe,100,20);check(capped.hit&&capped.hit->applied==130);
    auto forged=p;++forged.maximum_damage;check(!energy_blast_target(forged,foe,20,20).accepted);
    foe.damageable=false;check(!energy_blast_target(p,foe,20,20).accepted);
    std::cout<<"Energy Blast policy passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
