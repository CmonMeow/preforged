// NEW unregistered follow-on contract. Compile only with candidate headers.
#include "gw_attack_outcome_delivery.h"
#include <stdexcept>
#include <iostream>
#include <type_traits>
using namespace fw::gw::combat::weapon;
void check(bool value){if(!value)throw std::runtime_error("targetless attackmade canonical contract");}
int main(){static_assert(!std::is_constructible_v<AttackMade,AttackScope,std::uint64_t,std::uint64_t,unsigned,std::uint64_t>);AttackScope a{1,1,1},b{1,2,1};AttackOutcomeDelivery owner(a);
 const auto reserved=owner.reserve_action(a,1,100,330,7);check(bool(reserved));const auto action=owner.make_action(a,*reserved);check(bool(action));check(action->source_epoch()==7);check(!owner.make_action(a,*reserved));check(!owner.cancel_action(a,*reserved,100));
 check(!owner.qualify_hit(a,{},1,100));check(!owner.finish(a,{},1,AttackOutcome::cancelled,100));
 check(owner.release(a,b,2,100,AttackDelivery::melee));
 auto first=owner.observe_action_once(a,*action,1);check(first&&*first);auto duplicate=owner.observe_action_once(a,*action,1);check(duplicate&&!*duplicate);
 auto vigor=owner.observe_action_once(a,*action,2);check(vigor&&*vigor);auto spite=owner.observe_action_once(a,*action,4);check(spite&&*spite);
 check(owner.finish_action(a,*action,100));check(!owner.action_observed(a,*action,1));
 const auto held=owner.completion_frontier(a);check(held&&held->through()==1); // real contact2 holds producer frontier
 const auto miss=owner.finish(a,b,2,AttackOutcome::miss,100);check(bool(miss));check(owner.can_finish(a,4095)&&!owner.can_finish(a,4096));check(owner.acknowledge(a,*miss));
 const auto done=owner.completion_frontier(a);check(done&&done->through()==2);
 const auto aborted=owner.reserve_action(a,3,100,330,8);check(bool(aborted));check(owner.cancel_action(a,*aborted,100));check(!owner.make_action(a,*aborted));
 std::cout<<"PASS targetless made attack: same journal, separate observer grains, no invented hit\n";
}
