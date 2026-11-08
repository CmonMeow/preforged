#pragma once
#include "gw_heal_effects.h"
#include "gw_cooldown_bar_authority.h"
#include "gw_disable_reset.h"

namespace fw::gw::skills::effects {
struct GiftHealthPlan {
 unsigned rank=0,healing=15,energy=5;Time disable=10000,activation=750,aftercast=750,recharge=5000;
 float range=1248.f;
 auto operator<=>(const GiftHealthPlan&)const=default;
};
inline std::optional<GiftHealthPlan> gift_health(unsigned rank){
 const auto* r=find_skill(1121);
 if(rank>20||!r||r->profession!=3||r->attribute!=13||r->type!=5||r->special||r->energy_cost()!=5||r->health_sacrifice||r->adrenaline||r->activation!=.75f||r->aftercast!=.75f||r->recharge!=5||r->scale0!=15||r->scale15!=150||r->bonus0!=10||r->bonus15!=5||r->caster_body_resource!=1259||r->target_body_resource!=2077)return {};
 return GiftHealthPlan{rank,heal_scale(15,150,rank),5,1000ull*heal_scale(10,5,rank)};
}
// Returns raw spell healing, before the existing target healing modifiers and
// separate Divine Favor transaction. Eligibility and payment are World policy.
inline std::optional<unsigned> gift_health_amount(const GiftHealthPlan& p){
 const auto canonical=gift_health(p.rank);return canonical&&*canonical==p?std::optional(p.healing):std::nullopt;
}
// Independent World disable source. Bind to the authoritative completion bar,
// not a client snapshot. All Healing Prayers skills qualify, not only spells;
// Gift itself is explicitly excluded. Exact copies retain their own timers.
class GiftHealthDisables {
 struct Rule {Slot slot;Time applied=0,until=0;bool operator==(const Rule&)const=default;};
 Scope owner_;Time clock_=0;std::uint64_t revision_=1,last_event_=0;std::vector<Rule> rules_;
 bool owns(Scope s)const{return cooldown_owner(s)&&s==owner_;}
public:
 explicit GiftHealthDisables(Scope owner):owner_(owner){}
 bool operator==(const GiftHealthDisables&)const=default;
 std::uint64_t revision()const{return revision_;}
 std::optional<GiftHealthDisables> propose(Scope owner,const GiftHealthPlan& p,const CooldownBarSnapshot& bar,std::uint64_t event,Time now)const{
  if(!owns(owner)||!gift_health_amount(p)||!event||event<=last_event_||now<clock_||now>UINT64_MAX-p.disable||revision_==UINT64_MAX||bar.published.owner!=owner||!bar.published.order||!bar.revision||bar.published.at>now)return {};
  auto next=*this;std::erase_if(next.rules_,[&](const auto& r){return r.until<=now;});ResetSlots seen;
  for(const auto slot:bar.bar){
   if(!slot.skill){if(slot.copy)return {};continue;}
   if(!cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
   const auto* row=find_skill(slot.skill);if(row->attribute!=13||slot.skill==1121)continue;
   auto found=std::find_if(next.rules_.begin(),next.rules_.end(),[&](const auto& r){return r.slot==slot;});
   if(found==next.rules_.end()){if(next.rules_.size()>=128)return {};next.rules_.push_back({slot,now,now+p.disable});}
   else found->until=std::max(found->until,now+p.disable);
  }
  next.clock_=now;next.last_event_=event;++next.revision_;return next;
 }
 std::optional<Time> disabled_until(Scope owner,Slot slot,Time now)const{
  if(!owns(owner)||!cooldown_slot(slot)||now<clock_)return {};
  Time until=0;for(const auto& r:rules_)if(r.slot==slot&&r.applied<=now&&now<r.until)until=std::max(until,r.until);return until;
 }
 std::optional<GiftHealthDisables> propose_reset_slots(Scope owner,const std::vector<Slot>& slots,Time now)const{
  const auto valid=validated_reset_slots(slots);if(!owns(owner)||!valid||now<clock_||revision_==UINT64_MAX)return {};
  auto next=*this;std::erase_if(next.rules_,[&](const auto& r){return r.until<=now||valid->contains({r.slot.skill,r.slot.copy});});next.clock_=now;++next.revision_;return next;
 }
 void retire(){owner_={};rules_.clear();}
};
}
