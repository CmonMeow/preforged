#include "gw_shield_absorption.h"
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool x){if(!x)throw std::runtime_error("Shield Absorption transaction");}
int main(){
 for(unsigned rank=0;rank<=20;++rank){auto p=shield_absorption(rank);check(p&&p->duration==1000ull*interpolate(3,7,rank)&&p->step==5);}
 check(!shield_absorption(21));
 Scope owner{1,2,3},stale{1,2,4};Registry registry(owner);ShieldAbsorption state(owner);
 check(!state.prepare(registry,owner,1,100));
 check(registry.apply(owner,{1,7,1399,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
 auto first=*state.prepare(registry,owner,1,100);check(first.reduction==5);
 check(state.prepare(registry,owner,1,100)->reduction==5);
 check(!state.prepare(registry,stale,1,100));check(!state.prepare(registry,owner,0,100));
 auto forged=first;forged.reduction=10;check(!state.commit(registry,owner,forged,100));
 forged=first;forged.effect.skill=245;check(!state.commit(registry,owner,forged,100));
 check(state.commit(registry,owner,first,100));check(!state.commit(registry,owner,first,100));
 for(unsigned n=2;n<=100;++n){auto hit=*state.prepare(registry,owner,n,100+n);check(hit.reduction==5*n);check(state.commit(registry,owner,hit,100+n));}
 check(!state.prepare(registry,owner,101,199));
 auto pending=*state.prepare(registry,owner,101,300);
 check(registry.apply(owner,{2,8,1399,0,Category::enchantment,300,3300},Replace::same_skill).accepted);
 check(!state.commit(registry,owner,pending,300));auto renewed=*state.prepare(registry,owner,101,300);check(renewed.reduction==5);
 check(state.commit(registry,owner,renewed,300));check(!state.prepare(registry,owner,102,3300));
 check(bool(registry.remove(owner,2)));check(!state.prepare(registry,owner,102,500));
 // Same instance/cause/time/rank is still a new application on renewal.
 Registry r(owner);ShieldAbsorption counter(owner);
 const Entry e{10,7,1399,12,Category::enchantment,100,6100};
 check(r.apply(owner,e,Replace::same_skill).accepted);
 auto a=*counter.prepare(r,owner,1,100);check(counter.commit(r,owner,a,100));
 auto old=*counter.prepare(r,owner,2,100);check(old.reduction==10);
 check(r.apply(owner,e,Replace::same_skill).accepted);
 check(!counter.commit(r,owner,old,100));
 auto fresh=*counter.prepare(r,owner,2,100);check(fresh.reduction==5);
 check(!counter.commit(r,owner,fresh,101));check(counter.commit(r,owner,fresh,100));
 // Speculative copies advance independently; discarding one preserves original.
 auto speculative=counter;auto trial=*speculative.prepare(r,owner,3,101);
 check(speculative.commit(r,owner,trial,101));
 check(counter.prepare(r,owner,3,101)->reduction==10);
 check(speculative.prepare(r,owner,4,101)->reduction==15);
}

