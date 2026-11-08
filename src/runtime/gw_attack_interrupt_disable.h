#pragma once
#include "gw_attack_interrupts.h"
#include "gw_bow_self_disable.h"
#include "gw_disable_reset.h"
#include "gw_effect_registry.h"
namespace fw::gw::combat::weapon {
// An extra disable is composed with the interrupted action's settled recharge,
// not with a guessed catalog recharge or an unrelated existing disable.
struct InterruptDisablePlan {
 unsigned attack=0;std::uint64_t extra=20000;
 bool operator==(const InterruptDisablePlan&)const=default;
};
inline std::optional<InterruptDisablePlan> interrupt_disable_plan(unsigned skill){
 const auto* r=skills::find_skill(skill);
 if(!r||r->type!=14||r->activation!=(skill==399?.5f:0.f)||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 if(skill==340){if(r->attribute!=18||r->adrenaline!=150||r->scale0!=20||r->scale15!=20)return {};}
 else if(skill==399){if(r->attribute!=23||r->energy_cost()!=5||r->recharge!=10||r->scale0!=1||r->scale15!=16||r->bonus0!=20||r->bonus15!=20)return {};}
 else return {};
 return InterruptDisablePlan{skill};
}
struct InterruptedActionReceipt {
 skills::effects::Scope target;skills::Slot slot;
 std::uint64_t action=0,began=0,ends=0,interrupted_at=0,recharge_until=0;
};
// This ledger consumes only a successful interruption receipt from the owning
// action controller. It does not decide hit,protection,interrupt or recharge.
// No production caller is enabled until it can settle that receipt atomically.
class AttackInterruptDisables {
 using Time=std::uint64_t;
 struct Rule {skills::Slot slot;Time applied,until;};
 skills::effects::Scope owner_;Time clock_=0;std::uint64_t revision_=1;
 // Target controller supplies a monotonically increasing action sequence for
 // this lifetime. A high-water mark avoids an ever-growing replay set.
 std::vector<Rule> rules_;std::uint64_t last_action_=0;Time self_at_=0;std::uint64_t self_order_=0;std::set<std::uint64_t> self_events_;
 bool owns(skills::effects::Scope owner)const{return owner==owner_&&owner.instance&&owner.actor&&owner.actor<=65535&&owner.generation;}
public:
 explicit AttackInterruptDisables(skills::effects::Scope owner):owner_(owner){}
 std::uint64_t revision()const{return revision_;}
 std::optional<AttackInterruptDisables> propose(skills::effects::Scope owner,const InterruptDisablePlan& plan,
     const InterruptedActionReceipt& action,Time now)const{
  const auto canonical=interrupt_disable_plan(plan.attack);
  if(!owns(owner)||!canonical||*canonical!=plan||action.target!=owner||!action.action||action.action<=last_action_||
     !action.slot.skill||!skills::find_skill(action.slot.skill)||action.slot.copy>65535||action.began>=action.ends||
     now!=action.interrupted_at||now<action.began||now>=action.ends||now<clock_||action.recharge_until<now||
     action.recharge_until>UINT64_MAX-plan.extra||revision_==UINT64_MAX)return {};
  auto next=*this;std::erase_if(next.rules_,[&](const Rule& r){return r.until<=now;});
  const auto until=action.recharge_until+plan.extra;
  const auto same=std::find_if(next.rules_.begin(),next.rules_.end(),[&](const Rule& r){return r.slot==action.slot;});
  if(same!=next.rules_.end())same->until=std::max(same->until,until);
  else{if(next.rules_.size()>=128)return {};next.rules_.push_back({action.slot,now,until});}
  next.last_action_=action.action;next.clock_=now;++next.revision_;return next;
 }
 // Typed landed bow self-disables share this independent-disable ledger's
 // revision/reset/retirement layer. Event IDs may arrive in reverse order.
 std::optional<AttackInterruptDisables> propose_self_hit(skills::effects::Scope owner,const BowSelfDisablePlan& plan,
     const skills::effects::CooldownBarSnapshot& bar,skills::effects::CooldownStamp stamp,std::uint64_t event,Time now)const{
  const auto slots=bow_self_disabled_slots(plan,bar.bar);
  if(!owns(owner)||!slots||!bar.revision||!bar.published.order||bar.published.owner!=owner||bar.published.at>now||
     stamp.owner!=owner||stamp.order<=bar.published.order||stamp.order<=self_order_||stamp.at!=now||!event||now<clock_||
     now<self_at_||now>UINT64_MAX-plan.duration||revision_==UINT64_MAX||
     (self_at_==now&&(self_events_.contains(event)||self_events_.size()>=4096)))return {};
  auto next=*this;if(now!=self_at_)next.self_events_.clear();next.self_events_.insert(event);
  std::erase_if(next.rules_,[&](const Rule& r){return r.until<=now;});
  for(const auto slot:*slots){auto same=std::find_if(next.rules_.begin(),next.rules_.end(),[&](const Rule& r){return r.slot==slot;});
   if(same==next.rules_.end()){if(next.rules_.size()>=128)return {};next.rules_.push_back({slot,now,now+plan.duration});}
   else same->until=std::max(same->until,now+plan.duration);
  }
  next.clock_=next.self_at_=now;next.self_order_=stamp.order;++next.revision_;return next;
 }
 std::optional<Time> disabled_until(skills::effects::Scope owner,skills::Slot slot,Time now)const{
  if(!owns(owner)||!slot.skill||!skills::find_skill(slot.skill)||slot.copy>65535||now<clock_)return {};
  Time until=0;for(const auto& rule:rules_)if(rule.slot==slot&&rule.applied<=now&&now<rule.until)until=std::max(until,rule.until);
  return until;
 }
 std::optional<AttackInterruptDisables> propose_reset_slots(skills::effects::Scope owner,const std::vector<skills::Slot>& slots,Time now)const{
  const auto keys=skills::effects::validated_reset_slots(slots);if(!owns(owner)||!keys||now<clock_||revision_==UINT64_MAX)return {};
  auto next=*this;std::erase_if(next.rules_,[&](const Rule& r){return r.until<=now||keys->contains({r.slot.skill,r.slot.copy});});
  next.clock_=now;++next.revision_;return next; // Reset must not make a consumed action replayable.
 }
 void retire(){owner_={};rules_.clear();last_action_=self_order_=0;self_events_.clear();}
};
}
