#include "gw_natural_stride.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Natural Stride line "+std::to_string(at.line()));
}
int main()try{
    const Scope owner{1,2,3};
    for(unsigned rank=0;rank<=20;++rank){
        const auto p=natural_stride(rank);check(bool(p));
        check(p->duration==1000ull*interpolate(1,8,rank));
        check(p->movement==1.33f&&p->block_percent==50);
    }
    check(!natural_stride(21));
    check(natural_stride(0)->duration==1000&&natural_stride(15)->duration==8000&&natural_stride(20)->duration==10000);
    for(const auto kind:{Category::hex,Category::enchantment}){
        Registry registry(owner);check(registry.observe_applications(owner,1727));
        const unsigned incoming=kind==Category::hex?26:138;
        check(category(*fw::gw::skills::find_skill(incoming))==kind);
        check(registry.apply(owner,{11,99,incoming,0,kind,100,10000},Replace::independent).accepted);
        check(registry.take_applications(owner).empty());
        check(registry.apply(owner,{12,99,1727,15,Category::stance,1000,9000},Replace::category).accepted);
        check(bool(active_natural_stride(registry,owner,1000))); // Existing effect never ends it.
        check(!active_natural_stride(registry,owner,999));
        check(!active_natural_stride(registry,owner,9000));
        check(registry.apply(owner,{13,99,incoming,0,kind,1000,10000},Replace::same_skill).accepted);
        auto events=registry.take_applications(owner);check(events.size()==1&&events[0].observers.size()==1);
        auto stale=owner;++stale.generation;
        check(!natural_stride_application(registry,stale,events[0]));
        check(bool(natural_stride_application(registry,owner,events[0])));
        check(!natural_stride_application(registry,owner,events[0]));
        check(!active_natural_stride(registry,owner,1000));
        // An application observed against an older stance cannot end recast.
        check(registry.apply(owner,{14,99,1727,15,Category::stance,2000,10000},Replace::category).accepted);
        check(registry.apply(owner,{15,99,incoming,0,kind,2000,10000},Replace::same_skill).accepted);
        events=registry.take_applications(owner);check(events.size()==1);
        check(registry.apply(owner,{16,99,1727,15,Category::stance,2000,10000},Replace::category).accepted);
        check(!natural_stride_application(registry,owner,events[0]));
        check(bool(active_natural_stride(registry,owner,2000)));
    }
    std::cout<<"PASS Natural Stride canonical ranks, existing/new affliction distinction, same-tick order and stale instance guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

