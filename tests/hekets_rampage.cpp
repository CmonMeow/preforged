#include "gw_hekets_rampage.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills;using namespace fw::gw::skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Heket's Rampage line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){const auto plan=hekets_rampage(rank);
        check(plan&&plan->duration==1000ull*interpolate(5,11,rank)&&plan->attack_speed_percent==33);}
    check(!hekets_rampage(21));
    const Scope owner{1,25,1};auto stale=owner;++stale.generation;
    Registry original(owner);
    check(original.apply(owner,{9,25,344,0,Category::stance,50,5050},Replace::category).accepted);
    const auto app=prepare_hekets_rampage(original,owner,10,*hekets_rampage(0),100);
    check(app&&app->removed.size()==1&&app->removed[0].instance==9);
    check(original.lookup(owner,9).has_value()&&!original.lookup(owner,10));
    check(active_hekets_rampage(app->registry,owner,100)->instance==10);
    check(active_hekets_rampage(app->registry,owner,5099).has_value());
    check(!active_hekets_rampage(app->registry,owner,5100));
    check(!active_hekets_rampage(app->registry,stale,200));
    auto forged=*hekets_rampage(0);++forged.attack_speed_percent;
    check(!prepare_hekets_rampage(original,owner,10,forged,100));
    check(!prepare_hekets_rampage(original,owner,10,*hekets_rampage(0),UINT64_MAX));
    Slots slots{};slots[0]={1728,0};slots[1]={1953,1};slots[2]={281,0};
    CooldownBarAuthority authority(owner);const auto bar=*authority.publish(owner,slots,100);
    const auto spell=prepare_hekets_completed_skill(app->registry,owner,{281,0},bar,200);
    check(spell&&!spell->ended&&active_hekets_rampage(spell->registry,owner,200).has_value());
    const auto ended=prepare_hekets_completed_skill(app->registry,owner,{1953,1},bar,200);
    check(ended&&ended->ended&&ended->ended->instance==10);
    check(active_hekets_rampage(app->registry,owner,200).has_value());
    check(!active_hekets_rampage(ended->registry,owner,200));
    const auto duplicate=prepare_hekets_completed_skill(ended->registry,owner,{1953,1},bar,200);
    check(duplicate&&!duplicate->ended);
    check(!prepare_hekets_completed_skill(app->registry,owner,{1953,0},bar,200));
    check(!prepare_hekets_completed_skill(app->registry,stale,{1953,1},bar,200));
    const auto expired=prepare_hekets_completed_skill(app->registry,owner,{1953,1},bar,5100);
    check(expired&&!expired->ended);
    std::cout<<"PASS detached Heket's Rampage owned application, IAS policy and completion-time ending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
