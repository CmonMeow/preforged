#pragma once
#include "gw_skillbar.h"
#include "gw_effect_registry.h"
#include <map>
#include <set>
namespace fw::gw::skills::effects {
struct CooldownStamp {Scope owner;std::uint64_t order=0,at=0;bool operator==(const CooldownStamp&)const=default;};
struct CooldownCheckpointSource {CooldownStamp stamp;Slot slot;std::uint64_t until=0;};
inline bool cooldown_owner(Scope owner){return owner.instance&&owner.actor&&owner.actor<=65535&&owner.generation;}
inline bool cooldown_slot(Slot slot){const auto* row=find_skill(slot.skill);return row&&row->id&&row->type&&slot.copy<=65535;}
// Mint under World authority. Serial order is causal; sampled event times can
// regress when a thread waits for World. Never authenticate a client timestamp.
class CooldownOrderClock {
 Scope owner_;std::uint64_t order_=0;
public:
 explicit CooldownOrderClock(Scope owner,std::uint64_t initial=0):owner_(owner),order_(initial){if(!cooldown_owner(owner))throw std::invalid_argument("invalid cooldown owner");}
 std::optional<CooldownStamp> next(Scope owner,std::uint64_t at){if(owner!=owner_||order_==UINT64_MAX)return {};return CooldownStamp{owner_,++order_,at};}
};
// Session-owned replay of trusted World ordering. It stores local base recharge
// only: independent World disables remain in their existing authority layer.
class OrderedAttackCooldowns {
 using Key=std::pair<unsigned,unsigned>;
 struct Timer {CooldownStamp latest;std::uint64_t until=0,reset_order=0;bool operator==(const Timer&)const=default;};
 struct Receipt {CooldownStamp stamp;bool reset=false;std::vector<Slot> slots;std::uint64_t until=0;bool operator==(const Receipt&)const=default;};
 Scope owner_;std::uint64_t floor_=0,floor_at_=0,latest_at_=0;
 std::map<Key,Timer> timers_;std::map<std::uint64_t,Receipt> receipts_;
 std::map<Key,Receipt> checkpoint_sources_;
 static constexpr std::size_t max_receipts=4096,max_slots=128;
 enum class Admission {accept,duplicate,stale,invalid,full};
 Admission validate(const Receipt& event)const{
  if(event.stamp.owner!=owner_||!event.stamp.order)return Admission::invalid;
  if(event.stamp.order<=floor_)return Admission::stale;
  const auto exact=receipts_.find(event.stamp.order);
  if(exact!=receipts_.end())return exact->second==event?Admission::duplicate:Admission::invalid;
  if(receipts_.size()>=max_receipts)return Admission::full;
  std::size_t added=0;for(const auto slot:event.slots)if(!timers_.contains({slot.skill,slot.copy}))++added;
  return timers_.size()+added>max_slots?Admission::full:Admission::accept;
 }
public:
 enum class Result {accepted,duplicate,stale,invalid,full};
 explicit OrderedAttackCooldowns(Scope owner):owner_(owner){if(!cooldown_owner(owner))throw std::invalid_argument("invalid cooldown owner");}
 bool operator==(const OrderedAttackCooldowns&)const=default;
 bool needs_checkpoint()const{return receipts_.size()>=1024||timers_.size()>=96;}
 Result put(CooldownStamp stamp,Slot slot,std::uint64_t until){
  if(!cooldown_slot(slot)||until<stamp.at)return Result::invalid;
  const Receipt event{stamp,false,{slot},until};const auto admission=validate(event);
  if(admission==Admission::stale){
   const auto known=checkpoint_sources_.find({slot.skill,slot.copy});
   if(known!=checkpoint_sources_.end()&&known->second==event)return Result::duplicate;
  }
  if(admission!=Admission::accept)return static_cast<Result>(admission);
  receipts_.emplace(stamp.order,event);auto& timer=timers_[{slot.skill,slot.copy}];
  if(stamp.order>timer.latest.order){timer.latest=stamp;timer.until=until;}
  latest_at_=std::max(latest_at_,stamp.at);return Result::accepted;
 }
private:
 Result reset_selected(CooldownStamp stamp,std::vector<Slot> eligible,bool include_pets){
  if(eligible.size()>8)return Result::invalid;
  std::sort(eligible.begin(),eligible.end(),[](Slot a,Slot b){return std::pair{a.skill,a.copy}<std::pair{b.skill,b.copy};});
  for(std::size_t i=0;i<eligible.size();++i){const auto slot=eligible[i];const auto* row=find_skill(slot.skill);
   if(!cooldown_slot(slot)||(row->type!=14&&(!include_pets||row->type!=20))||(i&&eligible[i-1]==slot))return Result::invalid;
  }
  const Receipt event{stamp,true,eligible,0};const auto admission=validate(event);
  if(admission!=Admission::accept)return static_cast<Result>(admission);
  receipts_.emplace(stamp.order,event);
  for(const auto slot:eligible){auto& timer=timers_[{slot.skill,slot.copy}];timer.reset_order=std::max(timer.reset_order,stamp.order);}
  latest_at_=std::max(latest_at_,stamp.at);return Result::accepted;
 }
public:
 // Existing attack-only callers retain their strict native14 contract.
 Result reset_attacks(CooldownStamp stamp,std::vector<Slot> eligible){return reset_selected(stamp,std::move(eligible),false);}
 // Caller has already verified a skill-specific attack+pet predicate/bar.
 Result reset_attack_skills(CooldownStamp stamp,std::vector<Slot> eligible){return reset_selected(stamp,std::move(eligible),true);}
 std::optional<std::uint64_t> until(Scope owner,Slot slot,std::uint64_t now)const{
  if(owner!=owner_||!cooldown_slot(slot)||now<latest_at_)return {};
  const auto found=timers_.find({slot.skill,slot.copy});if(found==timers_.end())return 0;
  const auto& timer=found->second;
  return timer.latest.order>timer.reset_order&&timer.until>now?timer.until:0;
 }
 // Query only a successfully admitted source receipt. This lets a session
 // suppress an old control/completion processed after its reset was delivered.
 std::optional<bool> reset_supersedes(CooldownStamp source,Slot slot)const{
  if(source.owner!=owner_||!source.order||!cooldown_slot(slot))return {};
  const auto receipt=receipts_.find(source.order);const auto timer=timers_.find({slot.skill,slot.copy});
  const auto seeded=checkpoint_sources_.find({slot.skill,slot.copy});
  const Receipt* known=receipt!=receipts_.end()?&receipt->second:seeded!=checkpoint_sources_.end()?&seeded->second:nullptr;
  if(!known||known->stamp!=source||known->reset||known->slots.size()!=1||known->slots.front()!=slot||timer==timers_.end())return {};
  return timer->second.reset_order>=source.order||timer->second.latest.order>source.order;
 }
 // The caller must hold a World permit proving ALL source work and reset
 // deliveries through this fresh watermark are processed. Seed every active
 // attack timer, including off-bar/copied keys, before removing old barriers.
private:
 bool checkpoint_selected(CooldownStamp watermark,const std::vector<CooldownCheckpointSource>& active,bool include_pets){
  if(watermark.owner!=owner_||!watermark.order||watermark.order<floor_||watermark.at<latest_at_||active.size()>max_slots||
     (!receipts_.empty()&&watermark.order<=receipts_.rbegin()->first))return false;
  std::map<Key,Timer> timers;std::map<Key,Receipt> seeded;std::set<std::uint64_t> orders;
  for(const auto& source:active){const auto key=Key{source.slot.skill,source.slot.copy};const auto* row=find_skill(source.slot.skill);
   if(!cooldown_slot(source.slot)||(row->type!=14&&(!include_pets||row->type!=20))||source.stamp.owner!=owner_||!source.stamp.order||
      source.stamp.order>=watermark.order||source.stamp.at>watermark.at||source.until<=watermark.at||
      !orders.insert(source.stamp.order).second||seeded.contains(key))return false;
   const Receipt receipt{source.stamp,false,{source.slot},source.until};
   if(const auto known=receipts_.find(source.stamp.order);known!=receipts_.end()&&known->second!=receipt)return false;
   if(source.stamp.order<=floor_){const auto old=checkpoint_sources_.find(key);if(old==checkpoint_sources_.end()||old->second!=receipt)return false;}
   if(const auto old=timers_.find(key);old!=timers_.end()){
    if(old->second.reset_order>=source.stamp.order||old->second.latest.order>source.stamp.order)return false;
    if(old->second.latest.order==source.stamp.order&&(old->second.latest!=source.stamp||old->second.until!=source.until))return false;
   }
   timers.emplace(key,Timer{source.stamp,source.until,0});seeded.emplace(key,receipt);
  }
  if(watermark.order==floor_)return watermark.at==floor_at_&&receipts_.empty()&&timers==timers_&&seeded==checkpoint_sources_;
  timers_=std::move(timers);checkpoint_sources_=std::move(seeded);receipts_.clear();
  floor_=watermark.order;floor_at_=latest_at_=watermark.at;return true;
 }
public:
 bool checkpoint(CooldownStamp watermark,const std::vector<CooldownCheckpointSource>& active){return checkpoint_selected(watermark,active,false);}
 bool checkpoint_attack_skills(CooldownStamp watermark,const std::vector<CooldownCheckpointSource>& active){return checkpoint_selected(watermark,active,true);}
 // Only call after the authoritative channel confirms no event <= this known
 // stamp remains undelivered. Retain active timers; reject later stale packets.
 bool acknowledge(CooldownStamp stamp){
  if(stamp.owner!=owner_||!stamp.order)return false;
  if(stamp.order==floor_)return stamp.at==floor_at_;
  const auto found=receipts_.find(stamp.order);
  if(stamp.order<floor_||found==receipts_.end()||found->second.stamp!=stamp)return false;
  floor_=stamp.order;floor_at_=stamp.at;
  receipts_.erase(receipts_.begin(),receipts_.upper_bound(floor_));
  std::erase_if(timers_,[&](const auto& entry){const auto& timer=entry.second;return timer.latest.order<=floor_&&timer.reset_order<=floor_&&
    (timer.latest.order<=timer.reset_order||timer.until<=floor_at_);});
  checkpoint_sources_.clear();
  for(const auto& [key,timer]:timers_)if(timer.latest.order&&timer.latest.order<=floor_&&timer.latest.order>timer.reset_order&&timer.until>floor_at_)
   checkpoint_sources_.emplace(key,Receipt{timer.latest,false,{{key.first,key.second}},timer.until});
  return true;
 }
};
}
