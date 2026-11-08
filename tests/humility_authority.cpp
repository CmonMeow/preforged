#include "gw_humility_disables.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Humility authority pair regression");}
int main()try{
 const Scope caster{1,1,1},target{1,2,1};HumilityDisables a(caster),b(target);const auto plan=*signet_humility(12);
 const auto pair=humility_authority_pair(a,b,caster,target,plan,100);check(bool(pair));
 check(a.disabled_until(caster,281,100)==0&&b.disabled_until(target,5,100)==0);
 check(pair->caster.disabled_until(caster,281,100)==10100&&pair->caster.disabled_until(caster,3,100)==10100&&pair->caster.disabled_until(caster,62,100)==0);
 check(pair->target.disabled_until(target,5,100)==13100&&pair->target.disabled_until(target,259,100)==13100&&pair->target.disabled_until(target,23,100)==0);
 const auto reversed=humility_authority_pair(pair->target,pair->caster,target,caster,plan,200);check(bool(reversed));
 check(reversed->caster.disabled_until(target,259,200)==13100&&reversed->caster.disabled_until(target,281,200)==10200);
 check(reversed->target.disabled_until(caster,259,200)==13200&&reversed->target.disabled_until(caster,281,200)==10100);
 check(!humility_authority_pair(pair->caster,pair->target,{1,1,2},target,plan,200));
 check(!humility_authority_pair(pair->caster,pair->target,caster,target,plan,99));
 check(!humility_authority_pair(a,b,caster,target,plan,UINT64_MAX));
 check(pair->caster.disabled_until(caster,281,10100)==0&&pair->target.disabled_until(target,5,13100)==0);
 auto retired=pair->target;retired.retire();check(!retired.disabled_until(target,5,200));
 check(!humility_authority_pair(a,retired,caster,target,plan,200));
 check(a.disabled_until(caster,281,200)==0&&pair->target.disabled_until(target,5,200)==13100);
 std::cout<<"PASS immutable paired Humility authority,combined elite/self penalties,new-skill queries,expiry and stale/retired/clock/overflow rejection;World binding pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
