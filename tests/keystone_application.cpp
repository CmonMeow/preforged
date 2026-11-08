#include "gw_keystone_application.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Keystone application line "+std::to_string(at.line()));
}
int main()try{
 const Scope owner{1,2,3},other{1,3,4};Registry registry(owner);KeystoneCharges charges(owner);
 // Unrelated signet effects must survive renewal; this is not a stance.
 check(registry.apply(owner,{10,owner.actor,62,0,Category::other,100,30000},Replace::independent).accepted);
 const auto p=prepare_keystone_application(registry,charges,owner,11,1,*keystone_signet(12),200);
 check(p&&p->entry.kind==Category::other&&p->entry.expires==20200&&p->charges.snapshot()->effect==11&&p->charges.snapshot()->remaining==5);
 check(!registry.lookup(owner,11)&&!charges.snapshot());
 check(owned_keystone(p->registry,p->charges,owner,200)&&!owned_keystone(p->registry,p->charges,owner,20200));
 auto replaced=p->registry;
 check(replaced.apply(owner,{11,owner.actor,62,12,Category::other,200,20200},Replace::independent).accepted);
 check(!owned_keystone(replaced,p->charges,owner,200));
 auto changed=p->registry;auto wrong=p->entry;++wrong.rank;
 check(changed.apply(owner,wrong,Replace::independent).accepted&&!owned_keystone(changed,p->charges,owner,200));
 const auto renewed=prepare_keystone_application(p->registry,p->charges,owner,12,2,*keystone_signet(20),300);
 check(renewed&&renewed->change.removed.size()==1&&renewed->change.removed[0].instance==11);
 check(renewed->registry.lookup(owner,10)&&!renewed->registry.lookup(owner,11)&&renewed->charges.snapshot()->remaining==8&&renewed->charges.snapshot()->expires==20300);
 check(!prepare_keystone_application(p->registry,p->charges,owner,12,1,*keystone_signet(12),300));
 check(!prepare_keystone_application(registry,KeystoneCharges(other),owner,11,1,*keystone_signet(12),200));
 check(!prepare_keystone_application(p->registry,p->charges,owner,11,2,*keystone_signet(12),300));
 auto forged=*keystone_signet(12);++forged.charges;
 check(!prepare_keystone_application(registry,charges,owner,11,1,forged,200));
 check(!prepare_keystone_application(registry,charges,owner,11,1,*keystone_signet(12),UINT64_MAX));
 const auto zero=prepare_keystone_application(registry,charges,owner,11,1,*keystone_signet(0),200);
 check(zero&&zero->charges.snapshot()->remaining==0); // Icon policy remains unverified.
 std::cout<<"PASS Keystone timer/charge identity, renewal isolation, replay/scope/overflow rejection; no World publication claimed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
