#include "gw_diversion_authority.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Diversion authority regression");}
int main()try{
 const Scope owner{1,2,1};Registry registry(owner);DiversionAuthority authority(owner);
 check(registry.apply(owner,{7,3,30,12,Category::hex,100,6100},Replace::same_skill).accepted);
 const DiversionUse used{owner,8,{28,1},true,false};
 const auto proposal=prepare_diversion_use(registry,authority,used,200,20000,90000);
 check(proposal&&proposal->recharge_until==90000&&proposal->authority.until(owner,{28,1},200)==90000);
 check(authority.until(owner,{28,1},200)==0&&registry.lookup(owner,7));
 check(!prepare_diversion_use(registry,proposal->authority,used,201,20000,0));
 auto stale=used;stale.owner.generation=2;check(!prepare_diversion_use(registry,authority,stale,200,20000,0));
 check(!prepare_diversion_use(registry,authority,used,UINT64_MAX-100,20000,0));
 const auto reset=prepare_diversion_use(registry,authority,{owner,9,{22,0},true,false},200,0,0);
 check(reset&&!reset->recharge_until&&reset->authority.until(owner,{22,0},200)==0);
 check(!prepare_diversion_use(registry,reset->authority,{owner,9,{28,0},true,false},200,0,0));
 check(proposal->authority.until(owner,{28,1},90000)==0);
 check(!proposal->authority.until({1,2,2},{28,1},200));
 check(!proposal->authority.until(owner,{28,1},199));
 std::cout<<"PASS Diversion immutable registry/recharge proposal, one-use event, stale/time/overflow rejection, instant reset and preserved long deadline; World/copy-sharing policy pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
