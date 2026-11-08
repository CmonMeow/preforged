#pragma once
#include "gw_cooldown_bar_authority.h"
#include "gw_disable_reset.h"
namespace fw::gw::skills::effects {
// Recall end disable: every exact current-bar copy, including signets and Mesmer skills.
// Rules survive bar
// removal/reinsertion and explicit resets keep the original replay watermark.
class RecallDisables {
 struct Rule {Slot slot;Time applied=0,until=0;bool operator==(const Rule&)const=default;};
 Scope owner_;Time clock_=0;std::uint64_t revision_=1,last_event_=0;std::vector<Rule> rules_;
 bool owns(Scope owner)const{return cooldown_owner(owner)&&owner==owner_;}
public:
 explicit RecallDisables(Scope owner):owner_(owner){}
 bool operator==(const RecallDisables&)const=default;
 std::uint64_t revision()const{return revision_;}
 std::optional<RecallDisables> propose(Scope owner,const CooldownBarSnapshot& bar,std::uint64_t event,Time now)const{
  if(!owns(owner)||bar.published.owner!=owner||!bar.published.order||bar.published.at>now||!bar.revision||
     !event||event<=last_event_||now<clock_||now>UINT64_MAX-10000||revision_==UINT64_MAX)return {};
  auto next=*this;std::erase_if(next.rules_,[&](const auto& r){return r.until<=now;});ResetSlots seen;
  for(const auto slot:bar.bar){
   if(!slot.skill){if(slot.copy)return {};continue;}
   if(!cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};

   const auto old=std::find_if(next.rules_.begin(),next.rules_.end(),[&](const auto& r){return r.slot==slot;});
   if(old==next.rules_.end()){if(next.rules_.size()>=128)return {};next.rules_.push_back({slot,now,now+10000});}
   else old->until=std::max(old->until,now+10000);
  }
  next.clock_=now;next.last_event_=event;++next.revision_;return next;
 }
 std::optional<Time> disabled_until(Scope owner,Slot slot,Time now)const{
  if(!owns(owner)||!cooldown_slot(slot)||now<clock_)return {};
  Time result=0;for(const auto& r:rules_)if(r.slot==slot&&r.applied<=now&&now<r.until)result=std::max(result,r.until);return result;
 }
 std::optional<RecallDisables> propose_reset_slots(Scope owner,const std::vector<Slot>& slots,Time now)const{
  const auto keys=validated_reset_slots(slots);if(!owns(owner)||!keys||now<clock_||revision_==UINT64_MAX)return {};
  auto next=*this;std::erase_if(next.rules_,[&](const auto& r){return r.until<=now||keys->contains({r.slot.skill,r.slot.copy});});
  next.clock_=now;++next.revision_;return next;
 }
 void retire(){owner_={};rules_.clear();}
};
}

