#include "gw_dagger_successful_second_draft.h"
#include <type_traits>
#include <stdexcept>
#include <iostream>
using namespace fw::gw;using namespace combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("retained dual temporal group");}
int main(){
 static_assert(!std::is_constructible_v<DaggerSuccessfulSecondDraft,Scope,Scope,const AttackPermit&,const DaggerDualGroupDraft&,std::uint64_t,const std::shared_ptr<skills::effects::ContactCooldownContinuation>&>);
 Scope a{1,1,1},b{1,2,1};skills::effects::DaggerChain chain;check(chain.lead(b,1000)&&chain.offhand(b,1000));
 auto group=DaggerDualGroupDraft::begin(a,b,chain,775,1,1500);check(bool(group));
 AttackOutcomes ledger(a);check(ledger.release(a,b,1,1500,AttackDelivery::melee));
 const auto first=group->release(a,b,775,1,1500);check(first&&!(*first));
 const auto miss=ledger.finish(a,b,1,AttackOutcome::miss,1500);check(miss&&group->finish(*miss));
 check(!group->select_second(1499)&&group->select_second(2000));check(!group->matches(a,b,775,1500)&&group->matches(a,b,775,2000));
 check(!group->release(a,b,775,2,1999));check(ledger.release(a,b,2,2000,AttackDelivery::melee));
 const auto second=group->release(a,b,775,2,2000);check(second&&*second);
 const auto block=ledger.finish(a,b,2,AttackOutcome::block,2000);check(block&&group->finish(*block)&&group->complete());
 check(!group->finish(*block)&&!group->select_second(2500));
 std::cout<<"PASS retained dual two real event times and private World capability\n";
}
