#include "gw_diversion_prepared_use.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Diversion prepared-use regression");}
int main()try{
 const Scope owner{1,2,1};Registry registry(owner);DiversionAuthority authority(owner);
 check(registry.apply(owner,{7,3,30,12,Category::hex,100,6100},Replace::same_skill).accepted);
 const auto prepared=capture_diversion_use(registry,{owner,8,{67,0},true,false},200);check(bool(prepared));
 check(bool(registry.lookup(owner,7))); // Capturing a potential use is nonmutating.
 auto removed=registry;check(bool(removed.remove(owner,7)));
 const auto after_removal=finalize_diversion_use(removed,authority,*prepared,200,10000,0);
 check(after_removal&&after_removal->recharge_until==57200&&!after_removal->removed_instance);
 auto renewed=removed;check(renewed.apply(owner,{8,3,30,0,Category::hex,200,6200},Replace::same_skill).accepted);
 const auto after_renewal=finalize_diversion_use(renewed,authority,*prepared,200,10000,0);
 check(after_renewal&&after_renewal->recharge_until==57200&&after_renewal->registry.lookup(owner,8));
 check(!finalize_diversion_use(renewed,after_renewal->authority,*prepared,200,10000,0));
 const auto reset=capture_diversion_use(registry,{owner,9,{22,0},true,false},200);
 const auto after_reset=finalize_diversion_use(registry,authority,*reset,200,0,0);
 check(after_reset&&!after_reset->recharge_until&&after_reset->removed_instance==7);
 auto reused=removed;check(reused.apply(owner,{7,3,30,0,Category::hex,200,6200},Replace::same_skill).accepted);
 check(!finalize_diversion_use(reused,authority,*prepared,200,10000,0));
 check(!finalize_diversion_use(registry,authority,*prepared,201,10000,0));
 Registry foreign({1,2,2});check(!finalize_diversion_use(foreign,authority,*prepared,200,10000,0));
 check(authority.until(owner,{67,0},200)==0&&registry.lookup(owner,7));
 std::cout<<"PASS Diversion pre-effect snapshot survives used-skill hex removal, preserves newer Diversion, handles reset, rejects replay/instance reuse and leaves originals unchanged\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
