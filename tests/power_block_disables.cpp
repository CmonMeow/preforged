#include "gw_power_block_disables.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Power Block disable authority regression");}
int main()try{
 const Scope target{1,2,3},caster{1,1,4};PowerBlockDisables state(target);
 check(state.disabled_until(target,23,100)==0);
 check(state.apply(target,caster,*power_block(12),23,100));
 check(state.disabled_until(target,23,100)==10100&&state.disabled_until(target,28,100)==10100);
 check(state.disabled_until(target,14,100)==0);
 check(state.apply(target,caster,*power_block(0),23,200));
 check(state.disabled_until(target,28,200)==10100); // A shorter second disable cannot shorten the first.
 check(!state.apply(target,caster,*power_block(12),23,199));
 check(!state.disabled_until({1,2,4},23,200));
 check(!state.disabled_until(target,0,200)&&!state.disabled_until(target,23,199));
 check(state.disabled_until(target,28,10099)==10100&&state.disabled_until(target,28,10100)==0);
 check(!state.apply(target,target,*power_block(12),23,10200));
 check(!state.apply(target,caster,*power_block(12),23,UINT64_MAX));
 check(state.apply(target,{1,1,5},*power_block(12),23,10200));
 check(state.disabled_until(target,28,10200)==20200); // New source lifetime does not corrupt target authority.
 state.retire();check(!state.disabled_until(target,23,10200)&&!state.apply(target,caster,*power_block(12),23,10200));
 std::cout<<"PASS scoped Power Block authority,same-attribute queries independent of equipped slots,overlap/expiry,stale generations and retirement;World/Runtime binding pending\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
