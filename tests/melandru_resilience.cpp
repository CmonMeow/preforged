#include "gw_melandru_resilience.h"
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool value){if(!value)throw std::runtime_error("Melandru resilience");}
int main(){
 const Scope owner{1,1,1};Registry original(owner);
 for(unsigned rank=0;rank<=20;++rank){const auto p=melandru_resilience(rank);check(p&&p->duration==1000ull*interpolate(8,20,rank));}
 check(!melandru_resilience(21));const auto plan=*melandru_resilience(0);
 check(original.apply(owner,{9,1,11,0,Category::stance,0,5000},Replace::category).accepted);
 auto proposal=prepare_melandru_resilience(plan,original,owner,10,1000);
 check(proposal&&proposal->removed.size()==1&&original.lookup(owner,9)&&!original.lookup(owner,10));
 auto registry=proposal->registry;fw::gw::combat::conditions::State conditions;
 using Kind=fw::gw::combat::conditions::Kind;
 check(conditions.apply(Kind::poison,2,1000,2000));check(conditions.apply(Kind::blind,2,1000,4000));
 // Different causes for the same hex do not multiply this per-hex bonus.
 check(registry.apply(owner,{11,2,26,0,Category::hex,1000,4000},Replace::independent).accepted);
 check(registry.apply(owner,{12,3,26,0,Category::hex,1000,6000},Replace::independent).accepted);
 check(registry.apply(owner,{13,2,30,0,Category::hex,1000,5000},Replace::independent).accepted);
 auto quote=melandru_resilience_regeneration(registry,owner,conditions,1000);
 check(quote&&*quote==MelandruResilienceRegeneration{2,2,16,4});
 check(melandru_resilience_regeneration(registry,owner,conditions,3000)->health_pips==12);
 check(melandru_resilience_regeneration(registry,owner,conditions,5000)->health_pips==4);
 check(melandru_resilience_regeneration(registry,owner,conditions,9000)->health_pips==0);
 auto boundaries=melandru_resilience_boundaries(registry,owner,conditions,1000,10000);
 check(boundaries&&*boundaries==std::vector<Time>({3000,4000,5000,6000,9000}));
 check(!melandru_resilience_regeneration(registry,{1,1,2},conditions,1000));
 auto forged=plan;forged.duration++;check(!prepare_melandru_resilience(forged,original,owner,10,1000));
 check(!prepare_melandru_resilience(plan,original,owner,0,1000));
 check(!prepare_melandru_resilience(plan,original,owner,10,UINT64_MAX));
 check(!melandru_resilience_boundaries(registry,owner,conditions,2000,1000));
 check(registry.remove(owner,10).has_value());
 check(melandru_resilience_regeneration(registry,owner,conditions,1000)->health_pips==0);
}
