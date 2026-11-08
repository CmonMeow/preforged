#pragma once
#include "gw_diversion.h"
#include "gw_disable_reset.h"
namespace fw::gw::skills::effects {
// Candidate authority for the explicitly used skill/copy. Whether other copies
// share this extension needs retail evidence; this API does not grant that rule.
class DiversionAuthority {
 Scope owner_;Time clock_=0;std::uint64_t last_event_=0,revision_=1;
 std::map<std::pair<unsigned,unsigned>,Time> deadlines_;
public:
 explicit DiversionAuthority(Scope owner):owner_(owner){}
 bool record(const DiversionUse& use,Time now,std::optional<Time> deadline){
  if(revision_==UINT64_MAX||use.slot.copy>65535||use.owner!=owner_||!owner_.instance||!owner_.actor||owner_.actor>65535||!owner_.generation||
     !use.skill_used||!use.event||use.event<=last_event_||now<clock_||
     !use.slot.skill||!find_skill(use.slot.skill)||(deadline&&*deadline<now))return false;
  auto proposed=deadlines_;
  std::erase_if(proposed,[&](const auto& pair){return pair.second<=now;});
  if(deadline&&*deadline>now){
   const auto key=std::pair{use.slot.skill,use.slot.copy};
   if(!proposed.contains(key)&&proposed.size()>=128)return false;
   proposed[key]=std::max(proposed[key],*deadline);
  }
  deadlines_=std::move(proposed);last_event_=use.event;clock_=now;++revision_;return true;
 }
 std::uint64_t revision()const{return revision_;}
 std::optional<DiversionAuthority> propose_reset_slots(Scope owner,const std::vector<Slot>& slots,Time now)const{
  const auto keys=validated_reset_slots(slots);
  if(owner!=owner_||!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!keys||now<clock_||revision_==UINT64_MAX)return {};
  auto result=*this;for(const auto& key:*keys)result.deadlines_.erase(key);
  result.clock_=now;++result.revision_;return result;
 }
 std::optional<Time> until(Scope owner,skills::Slot slot,Time now)const{
  if(owner!=owner_||!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!slot.skill||!find_skill(slot.skill)||slot.copy>65535||now<clock_)return {};
  const auto found=deadlines_.find({slot.skill,slot.copy});
  return found!=deadlines_.end()&&found->second>now?found->second:0;
 }
};
struct DiversionTransaction {
 DiversionUseProposal effect;
 DiversionAuthority authority;
 std::optional<Time> recharge_until;
};
inline std::optional<DiversionTransaction> prepare_diversion_use(const Registry& registry,
 const DiversionAuthority& authority,const DiversionUse& use,Time now,Time base,Time existing){
 const auto effect=diversion_use(registry,use,now);if(!effect)return {};
 const auto deadline=diversion_recharge_deadline(now,base,effect->additional_recharge,existing);if(!deadline)return {};
 auto proposed=authority;
 const auto extension=effect->additional_recharge?deadline:std::nullopt;
 if(!proposed.record(use,now,extension))return {};
 return DiversionTransaction{*effect,std::move(proposed),extension};
}
}
