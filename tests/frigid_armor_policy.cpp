#include "gw_frigid_armor.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool good){if(!good)throw std::runtime_error("Frigid Armor policy");}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        const auto p=frigid_armor(rank);check(bool(p));
        check(p->physical_armor==interpolate(10,40,rank));
        check(p->duration==1000ull*interpolate(10,25,rank));
        check(p->burning_immunity);
    }
    check(!frigid_armor(21));
    const Scope owner{1,25,1},stale{1,25,2};
    Registry baseline(owner);
    const auto plan=*frigid_armor(0);
    check(!prepare_frigid_armor(baseline,stale,69,plan,100));
    check(!prepare_frigid_armor(baseline,owner,0,plan,100));
    auto forged=plan;forged.physical_armor=41;
    check(!prepare_frigid_armor(baseline,owner,69,forged,100));
    check(!baseline.newest_skill(owner,1261,100));
    auto first=prepare_frigid_armor(baseline,owner,69,plan,100);
    check(bool(first)&&first->replaced.empty());
    check(!active_frigid_armor(baseline,owner,100));
    check(active_frigid_armor(first->registry,owner,100)==plan);
    check(!active_frigid_armor(first->registry,owner,10100));
    auto second=prepare_frigid_armor(first->registry,owner,70,plan,500);
    check(bool(second)&&second->replaced.size()==1&&
        second->replaced.front().instance==69);
    check(active_frigid_armor(second->registry,owner,500)==plan);
    check(!second->registry.lookup(owner,69));
    check(!active_frigid_armor(second->registry,stale,500));
    std::cout<<"PASS Frigid Armor canonical policy and transactional renewal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
