#include "gw_hex_breaker.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Hex Breaker application policy regression");}
int main()try{
 check(hex_breaker(0)->duration==5000&&hex_breaker(12)->duration==65000&&hex_breaker(12)->damage==39&&hex_breaker(20)->damage==58&&!hex_breaker(21));
 const Scope owner{1,2,1},caster{1,3,1};Registry registry(owner);
 check(registry.apply(owner,{1,2,10,12,Category::stance,100,65100},Replace::category).accepted);
 check(!take_hex_breaker(registry,owner,caster,151,200)&&registry.lookup(owner,1));
 check(!take_hex_breaker(registry,{1,2,2},caster,28,200)&&registry.lookup(owner,1));
 check(!take_hex_breaker(registry,owner,{2,3,1},28,200)&&registry.lookup(owner,1));
 auto proposed=registry;const auto reaction=take_hex_breaker(proposed,owner,caster,28,200);
 check(reaction&&reaction->damage==39&&reaction->stance_instance==1&&reaction->owner==owner&&reaction->caster==caster);
 check(registry.lookup(owner,1)&&!proposed.lookup(owner,1)); // Speculative failure does not consume authoritative stance.
 registry=std::move(proposed);check(!take_hex_breaker(registry,owner,caster,28,200));
 check(registry.apply(owner,{2,2,10,0,Category::stance,300,5300},Replace::category).accepted);
 check(!take_hex_breaker(registry,owner,caster,28,299));
 check(!take_hex_breaker(registry,owner,caster,28,5300));
 check(registry.apply(owner,{3,3,10,12,Category::stance,5400,70400},Replace::category).accepted);
 check(!take_hex_breaker(registry,owner,caster,28,5500)&&registry.lookup(owner,3));
 check(registry.apply(owner,{4,2,10,12,Category::stance,5600,{}},Replace::category).accepted);
 check(!take_hex_breaker(registry,owner,caster,28,5700)&&registry.lookup(owner,4));
 std::cout<<"PASS Hex Breaker selected-hex registry transaction,one-use consumption,nonhex/stale/expiry exclusion and speculative rollback;World retaliation/application integration pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
