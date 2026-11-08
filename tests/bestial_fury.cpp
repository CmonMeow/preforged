#include "gw_bestial_fury.h"
#include <stdexcept>
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::skills;
using namespace fw::gw::skills::effects;
namespace {
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bestial Fury line "+std::to_string(at.line()));
}
}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=bestial_fury(rank);
        check(plan&&plan->duration==1000ull*interpolate(5,11,rank)&&
              plan->non_attack_disable==5000&&plan->attack_speed_percent==25);
    }
    check(!bestial_fury(21));
    check(bestial_fury_non_attack(1209));
    check(!bestial_fury_non_attack(395)); // Barrage is an attack skill.
    const Scope owner{1,5,1};
    BestialFuryDisables initial(owner);
    CooldownBarSnapshot bar{};
    bar.published.owner=owner;bar.published.order=1;bar.published.at=100;
    bar.revision=1;
    bar.bar={Slot{1209,0},Slot{1209,1},Slot{395,0},Slot{1118,0}};
    const auto plan=*bestial_fury(12);
    const auto first=initial.propose(owner,plan,bar,1,100);
    check(first&&first->revision()==2);
    check(first->disabled_until(owner,{1209,0},101)==5100);
    check(first->disabled_until(owner,{1209,1},101)==5100);
    check(first->disabled_until(owner,{1118,0},101)==5100);
    check(first->disabled_until(owner,{395,0},101)==0);
    check(first->disabled_until(owner,{1118,1},101)==0);
    check(first->disabled_until(owner,{1118,0},5100)==0);
    check(!first->propose(owner,plan,bar,1,101));
    auto forged=plan;++forged.attack_speed_percent;
    check(!first->propose(owner,forged,bar,2,101));
    auto duplicate=bar;duplicate.bar[2]=duplicate.bar[0];
    check(!first->propose(owner,plan,duplicate,2,101));
    auto reset=first->propose_reset_slots(owner,{{1118,0}},102);
    check(reset&&reset->disabled_until(owner,{1118,0},102)==0&&
          reset->disabled_until(owner,{1209,1},102)==5100);
    auto renewed=first->propose(owner,plan,bar,2,200);
    check(renewed&&renewed->disabled_until(owner,{1118,0},201)==5200);
    auto retired=*renewed;retired.retire();
    check(!retired.propose(owner,plan,bar,3,202));
    std::cout<<"PASS Bestial Fury 2022 plan and exact non-attack disables; not bound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
