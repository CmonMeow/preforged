// NEW detached foundation only. No Scythe geometry/World producer is enabled.
#include "gw_attack_outcome_delivery.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>
#include <type_traits>
using namespace fw::gw;using namespace combat::weapon;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("ordinary multiple-contact canonical authority line "+std::to_string(at.line()));}
int main()try{
 const AttackScope source{1,1,7},primary{1,2,4},secondary{1,3,5},third{1,4,6};AttackOutcomeDelivery owner(source);
 static_assert(!std::is_constructible_v<AttackMade,AttackScope,std::uint64_t,std::uint64_t,unsigned,std::uint64_t>);
 check(!owner.reserve_action(source,1,100,0,9)); // skill path still rejects ID0
 check(!owner.reserve_ordinary_action({1,1,8},1,100,9));
 const auto action=owner.reserve_ordinary_action(source,1,100,9);check(bool(action));check(action->skill()==0&&action->source_epoch()==9);
 check(owner.release(source,primary,2,100,AttackDelivery::melee));check(owner.release(source,secondary,3,100,AttackDelivery::melee));check(owner.release(source,third,4,100,AttackDelivery::melee));
 const auto made=owner.make_action(source,*action);check(bool(made)&&made->skill()==0);check(!owner.cancel_action(source,*action,100));
 // The World adapter can consume source Empathy without reaction and use each
 // canonical recipient's observer independently. Vigor remains one source bit.
 auto source_hex=owner.observe_action_once(source,*made,1);check(source_hex&&*source_hex);
 auto vigor=owner.observe_action_once(source,*made,2);check(vigor&&*vigor);vigor=owner.observe_action_once(source,*made,2);check(vigor&&!*vigor);
 unsigned event=2;for(const auto target:{primary,secondary,third}){
  auto first=owner.observe_release_once(source,target,event,100,1);check(first&&*first);auto duplicate=owner.observe_release_once(source,target,event,100,1);check(duplicate&&!*duplicate);++event;
 }
 // Three attempted contacts remain independently miss/block/qualified hit;
 // action identity cannot itself qualify as any recipient or emit a hit.
 check(!owner.qualify_hit(source,primary,1,100));check(!owner.finish(source,primary,1,AttackOutcome::hit,100));
 const auto miss=owner.finish(source,primary,2,AttackOutcome::miss,100);const auto block=owner.finish(source,secondary,3,AttackOutcome::block,100);
 check(miss&&block);const auto qualified=owner.qualify_hit(source,third,4,100);check(bool(qualified));
 combat::Actor target{third.actor,third.generation,combat::Health{100,100},{},{},true};const auto hit=combat::apply(target,{combat::Kind::armor_ignoring,10,20,335},true);
 check(hit.reason==combat::Reason::accepted);
 check(bool(owner.commit_damage(source,third,4,hit,100)));const auto completed=owner.finish(source,third,4,AttackOutcome::hit,100);check(completed&&completed->before_damage==100);
 check(owner.finish_action(source,*made,100));check(!owner.action_observed(source,*made,2));
 const auto held=owner.completion_frontier(source);check(held&&held->through()==1); // real buffered recipients hold retirement
 check(owner.acknowledge(source,*block)&&owner.acknowledge(source,*miss)&&owner.acknowledge(source,*completed));
 const auto done=owner.completion_frontier(source);check(done&&done->through()==4);
 // Skill actions still use the same canonical capacity and IDs afterwards.
 const auto skill=owner.reserve_action(source,5,101,330,10);check(bool(skill));check(owner.cancel_action(source,*skill,101));
 const auto ordinary=owner.reserve_ordinary_action(source,6,102,11);check(bool(ordinary));check(owner.retire(source));check(!owner.make_action(source,*ordinary));
 std::cout<<"PASS ordinary action/recipient shared journal, independent miss/block/hit, once observer grains, frontier and epoch authority\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
