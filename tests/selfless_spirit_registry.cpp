#include "gw_selfless_spirit_registry.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("Selfless registry regression");}
int main()try{
    const Scope owner{1,25,7},stale{1,25,8};Registry current(owner);
    auto first=stage_selfless_spirit_registry(current,owner,1,*selfless_spirit(1952,0),100);
    check(first&&current.size()==0&&first->registry.size()==1&&first->added.expires==15100);
    check(first->registry.apply(owner,{2,25,180,0,Category::enchantment,100,100000},Replace::independent).accepted);
    auto second=stage_selfless_spirit_registry(first->registry,owner,3,*selfless_spirit(2095,12),200);
    check(second&&second->removed.size()==1&&second->removed[0].instance==1&&second->added.expires==20200);
    check(first->registry.lookup(owner,1)&&!second->registry.lookup(owner,1));
    check(second->registry.lookup(owner,2)&&second->registry.lookup(owner,3));
    auto live=live_selfless_spirit_entry(second->registry,owner,20199);
    check(live&&*live&&(**live).skill==2095);
    auto expired=live_selfless_spirit_entry(second->registry,owner,20200);
    check(expired&&!(*expired));
    auto early=live_selfless_spirit_entry(second->registry,owner,199);check(early&&!(*early));
    check(!live_selfless_spirit_entry(second->registry,stale,200));
    check(!stage_selfless_spirit_registry(second->registry,stale,4,*selfless_spirit(1952),300));
    check(!stage_selfless_spirit_registry(second->registry,owner,2,*selfless_spirit(1952),300));
    check(!stage_selfless_spirit_registry(second->registry,owner,4,*selfless_spirit(1952),UINT64_MAX-1));
    auto forged=*selfless_spirit(1952);++forged.duration_ms;
    check(!stage_selfless_spirit_registry(second->registry,owner,4,forged,300));
    check(second->registry.remove(owner,3).has_value());
    auto stripped=live_selfless_spirit_entry(second->registry,owner,300);check(stripped&&!(*stripped));
    check(second->registry.lookup(owner,2).has_value());
    auto after_expiry=stage_selfless_spirit_registry(first->registry,owner,4,*selfless_spirit(1952),15100);
    check(after_expiry&&after_expiry->removed.size()==1&&after_expiry->removed[0].instance==1);
    check(after_expiry->registry.lookup(owner,2).has_value());
    auto cleared=after_expiry->registry.clear(owner);check(cleared.size()==2);
    auto dead=live_selfless_spirit_entry(after_expiry->registry,owner,15100);check(dead&&!(*dead));
    std::cout<<"Selfless Spirit detached registry lifecycle passed; unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
