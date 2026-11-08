#pragma once
#include "gw_attack_outcomes.h"
#include <type_traits>
namespace fw::gw::combat::weapon {
class AttackOutcomeDelivery;
// Consumer retirement is a different capability from producer completion.
// A finished-but-buffered outcome is not acknowledged merely by finish().
class AttackConsumerFrontier {
 AttackScope owner_;std::uint64_t through_=0,at_=0;
 AttackConsumerFrontier(AttackScope owner,std::uint64_t through,std::uint64_t at):owner_(owner),through_(through),at_(at){}
 friend class AttackOutcomeDelivery;
public:
 AttackScope owner()const{return owner_;}
 std::uint64_t through()const{return through_;}
 std::uint64_t at()const{return at_;}
};
// Owns one authoritative AttackOutcomes ledger, not a shadow ledger beside it.
// World enrolls this delivery queue before exposing charged contacts, and routes
// every relevant producer finish through this single owner.
class AttackOutcomeDelivery {
 struct Completed {ResolvedAttack outcome;std::uint64_t sequence=0;};
 AttackScope owner_;AttackOutcomes producer_;std::map<std::uint64_t,Completed>completed_;std::uint64_t sequence_=0;
 static bool same(const ResolvedAttack&a,const ResolvedAttack&b){return a.attacker==b.attacker&&a.target==b.target&&a.event==b.event&&a.released==b.released&&a.at==b.at&&a.outcome==b.outcome&&a.before_damage==b.before_damage&&a.delivery==b.delivery&&a.skill_failed==b.skill_failed;}
public:
 explicit AttackOutcomeDelivery(AttackScope owner):owner_(owner),producer_(owner){}
 bool release(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at,AttackDelivery delivery=AttackDelivery::unknown){return producer_.release(caller,target,event,at,delivery);}
 std::optional<AttackActionReservation> reserve_action(AttackScope caller,std::uint64_t event,std::uint64_t at,unsigned skill,std::uint64_t source_epoch=0){return producer_.reserve_action(caller,event,at,skill,source_epoch);}
 std::optional<AttackActionReservation> reserve_ordinary_action(AttackScope caller,std::uint64_t event,std::uint64_t at,std::uint64_t source_epoch=0){return producer_.reserve_ordinary_action(caller,event,at,source_epoch);}
 std::optional<AttackMade> make_action(AttackScope caller,const AttackActionReservation&action){return producer_.make_action(caller,action);}
 bool cancel_action(AttackScope caller,const AttackActionReservation&action,std::uint64_t at){return producer_.cancel_action(caller,action,at);}
 std::optional<bool> action_observed(AttackScope caller,const AttackMade&action,unsigned observer)const{return producer_.action_observed(caller,action,observer);}
 std::optional<bool> observe_action_once(AttackScope caller,const AttackMade&action,unsigned observer){return producer_.observe_action_once(caller,action,observer);}
 // No delivery queue entry: caller closes only after synchronous source
 // observers. Existing unfinished contact/buffered frontiers still hold back.
 bool declare_ordinary_contacts(AttackScope caller,const AttackMade& action,unsigned count,std::uint64_t epoch){return producer_.declare_ordinary_contacts(caller,action,count,epoch);}
 bool release_action_contact(AttackScope caller,const AttackMade& action,AttackScope target,std::uint64_t event,std::uint64_t at,unsigned ordinal,std::uint64_t epoch){return producer_.release_action_contact(caller,action,target,event,at,ordinal,epoch);}
 std::optional<unsigned> action_contact_ordinal(AttackScope caller,const AttackMade& action,AttackScope target,std::uint64_t event)const{return producer_.action_contact_ordinal(caller,action,target,event);}
 bool finish_action(AttackScope caller,const AttackMade&action,std::uint64_t at){return producer_.finish_action(caller,action,at);}
 std::optional<bool> release_observed(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at,unsigned observer)const{return producer_.release_observed(caller,target,event,at,observer);}
 std::optional<bool> observe_release_once(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at,unsigned observer){return producer_.observe_release_once(caller,target,event,at,observer);}
 std::optional<QualifiedHit> qualify_hit(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at){return producer_.qualify_hit(caller,target,event,at);}
 std::optional<CommittedAttackDamage> commit_damage(AttackScope caller,AttackScope target,std::uint64_t event,const combat::Hit& hit,std::uint64_t at){
  if(!can_finish(caller))return {};return producer_.commit_damage(caller,target,event,hit,at);
 }
 bool can_finish(AttackScope caller,std::size_t count=1)const{return caller==owner_&&count<=4096&&completed_.size()<=4096-count&&sequence_<=UINT64_MAX-count;}
 std::optional<ResolvedAttack> finish(AttackScope caller,AttackScope target,std::uint64_t event,AttackOutcome outcome,std::uint64_t at,bool skill_failed=false){
  // Reserve delivery capacity before mutating the producer. The production
  // adapter must also preflight can_finish under its mutex before Health damage.
  if(!can_finish(caller)||!event||completed_.contains(event))return {};
  const auto[slot,inserted]=completed_.try_emplace(event,Completed{});if(!inserted)return {};
  static_assert(std::is_nothrow_copy_assignable_v<ResolvedAttack>&&std::is_nothrow_copy_constructible_v<std::optional<ResolvedAttack>>);
  try{
   const auto result=producer_.finish(caller,target,event,outcome,at,skill_failed);if(!result){completed_.erase(slot);return {};}
   slot->second.outcome=*result;slot->second.sequence=++sequence_;return result;
  }catch(...){completed_.erase(slot);throw;}
 }
 std::optional<std::vector<ResolvedAttack>> buffered(AttackScope caller)const{
  if(caller!=owner_)return {};
  std::vector<const Completed*>ordered;ordered.reserve(completed_.size());for(const auto&[id,event]:completed_)ordered.push_back(&event);
  std::sort(ordered.begin(),ordered.end(),[](const auto*a,const auto*b){return a->sequence<b->sequence;});
  std::vector<ResolvedAttack>result;result.reserve(ordered.size());for(const auto*event:ordered)result.push_back(event->outcome);return result;
 }
 // Acknowledge only AFTER the consumer commits, or explicitly handles the event
 // as no effect (e.g. retired target). It is not an instruction to apply a skill.
 bool acknowledge(AttackScope caller,const ResolvedAttack&event){
  if(caller!=owner_)return false;const auto found=completed_.find(event.event);
  if(found==completed_.end()||!same(found->second.outcome,event))return false;completed_.erase(found);return true;
 }
 std::optional<AttackCompletionFrontier> producer_frontier(AttackScope caller)const{return producer_.completion_frontier(caller);}
 std::optional<AttackConsumerFrontier> completion_frontier(AttackScope caller)const{
  const auto frontier=producer_.completion_frontier(caller);if(!frontier)return {};
  const auto through=completed_.empty()?frontier->through():std::min(frontier->through(),completed_.begin()->first-1);
  return AttackConsumerFrontier(owner_,through,frontier->at());
 }
 void discard_target(AttackScope target){producer_.discard_target(target);} // Already completed events still require acknowledgement.
 bool retire(AttackScope caller){return producer_.retire(caller);} // Buffered delivery remains visible after retirement.
 std::size_t size()const{return producer_.size();}
 std::size_t buffered_size()const{return completed_.size();}
 std::uint64_t clock()const{return producer_.clock();}
};
}
