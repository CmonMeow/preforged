#pragma once
#include "gw_determined_shot.h"
#include <deque>

namespace fw::gw::combat::weapon {
struct DeterminedNotice {
 skills::effects::CooldownStamp stamp;
 DeterminedRecharge recharge;
 bool operator==(const DeterminedNotice&)const=default;
};
// World-owned retained channel. Stage a copy before committing disable changes,
// then install both under the World mutex. Reading never consumes a notice;
// only the owning session's exact acknowledged stamp retires a prefix.
class DeterminedDelivery {
 AttackScope owner_;
 std::deque<DeterminedNotice> pending_;
 std::optional<skills::effects::CooldownStamp> last_,acknowledged_;
 std::set<std::uint64_t> impact_events_;
 static constexpr std::size_t capacity=64;
public:
 enum class Result {accepted,duplicate,stale,invalid,full};
 explicit DeterminedDelivery(AttackScope owner):owner_(owner){
  if(!valid_recharge_scope(owner))throw std::invalid_argument("invalid recharge delivery owner");
 }
 bool operator==(const DeterminedDelivery&)const=default;
 bool full()const{return pending_.size()==capacity;}
 Result publish(const DeterminedNotice& notice){
  const auto& stamp=notice.stamp;
  const auto proposal=prepare_determined_recharge(notice.recharge.before,notice.recharge.trigger);
  if(stamp.owner!=owner_||!stamp.order||!proposal||*proposal!=notice.recharge||
     notice.recharge.trigger.owner!=owner_||stamp.at!=notice.recharge.trigger.at)return Result::invalid;
  if(acknowledged_&&stamp.order<=acknowledged_->order)return Result::stale;
  for(const auto& old:pending_)if(old.stamp.order==stamp.order)
   return old==notice?Result::duplicate:Result::invalid;
  if(last_&&(stamp.order<=last_->order||stamp.at<last_->at))return Result::invalid;
  if(last_&&stamp.at==last_->at&&impact_events_.contains(notice.recharge.trigger.event))return Result::invalid;
  if(last_&&stamp.at==last_->at&&impact_events_.size()==4096)return Result::full;
  if(pending_.size()==capacity)return Result::full;
  pending_.push_back(notice);
  if(!last_||stamp.at>last_->at)impact_events_.clear();
  impact_events_.insert(notice.recharge.trigger.event);last_=stamp;return Result::accepted;
 }
 std::optional<std::vector<DeterminedNotice>> pending(AttackScope owner)const{
  if(owner!=owner_)return {};return std::vector<DeterminedNotice>(pending_.begin(),pending_.end());
 }
 std::optional<std::uint64_t> acknowledged_order(AttackScope owner)const{
  if(owner!=owner_)return {};return acknowledged_?acknowledged_->order:0;
 }
 bool acknowledge(skills::effects::CooldownStamp stamp){
  if(stamp.owner!=owner_||!stamp.order)return false;
  if(acknowledged_&&stamp==*acknowledged_)return true;
  const auto found=std::find_if(pending_.begin(),pending_.end(),[&](const auto& entry){return entry.stamp==stamp;});
  if(found==pending_.end())return false;
  pending_.erase(pending_.begin(),std::next(found));acknowledged_=stamp;return true;
 }
};
}
