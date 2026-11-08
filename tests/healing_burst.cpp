#include "gw_healing_burst.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills;
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Healing Burst line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        const auto p=healing_burst(rank,12);
        check(p&&p->heal==10+rank*10&&p->divine_party_heal==38&&
            p->smiting_disable==20000&&p->target_range==1248.f&&
            p->party_range==1000.f);
    }
    check(!healing_burst(21,0)&&!healing_burst(0,21));
    const auto body=particles::target_body_2022(2,1118);
    check(body&&*body==fw::gw::combat::detail::integer(21,2,1256));
    const auto recipient_plan=*healing_burst(12,12);
    check(healing_burst_recipient(recipient_plan,true,false,0)==168);
    check(healing_burst_recipient(recipient_plan,true,true,0)==168);
    check(healing_burst_recipient(recipient_plan,false,true,1000)==38);
    check(healing_burst_recipient(recipient_plan,false,true,1001)==0);
    check(healing_burst_recipient(recipient_plan,false,false,100)==0);
    check(!healing_burst_recipient(recipient_plan,false,true,-1));
    const Scope owner{1,25,1};
    Slots slots{};
    slots[0]={1118,0};slots[1]={240,0};slots[2]={240,1};
    slots[3]={281,0};slots[4]={294,0};slots[5]={247,0};
    CooldownBarAuthority authority(owner);
    const auto bar=*authority.publish(owner,slots,100);
    HealingBurstDisables original(owner);
    const auto plan=*healing_burst(12,12);
    const auto first=original.propose(owner,plan,bar,1,200);
    check(first.has_value());
    for(unsigned n:{1u,2u,4u,5u}){
        check(first->disabled_until(owner,slots[n],200)==20200);
        check(first->disabled_until(owner,slots[n],20199)==20200);
        check(first->disabled_until(owner,slots[n],20200)==0);
    }
    check(first->disabled_until(owner,slots[0],200)==0);
    check(first->disabled_until(owner,slots[3],200)==0);
    check(first->disabled_until(owner,{240,2},200)==0);
    check(!first->propose(owner,plan,bar,1,300));
    const auto renewed=first->propose(owner,plan,bar,2,300);
    check(renewed&&renewed->disabled_until(owner,slots[1],300)==20300);
    const auto reset=renewed->propose_reset_slots(owner,{slots[1],slots[4]},400);
    check(reset&&reset->disabled_until(owner,slots[1],400)==0&&
        reset->disabled_until(owner,slots[2],400)==20300&&
        reset->disabled_until(owner,slots[4],400)==0);
    auto forged=plan;forged.smiting_disable=19999;
    check(!original.propose(owner,forged,bar,1,200));
    forged=plan;forged.target_range=1250;
    check(!original.propose(owner,forged,bar,1,200));
    forged=plan;forged.divine_party_heal=999;
    check(!original.propose(owner,forged,bar,1,200));
    auto corrupt=bar;corrupt.bar[5]=corrupt.bar[1];
    check(!original.propose(owner,plan,corrupt,1,200));
    check(!original.propose({1,25,2},plan,bar,1,200));
    check(!original.propose(owner,plan,bar,0,200));
    check(!original.propose(owner,plan,bar,1,UINT64_MAX));
    auto retired=*first;retired.retire();
    check(!retired.disabled_until(owner,slots[1],200));
    std::cout<<"Healing Burst rank/Divine plan and Smiting disable tests pass\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
