#pragma once
#include "gw_recall.h"
#include <map>
namespace fw::gw::skills::effects {
struct PendingRecallEnd {RecallAnchor anchor;Time at=0;std::uint64_t event=0;};
// Owned removal records its work before reactive callbacks. Preparation never
// consumes a record; only a successful World commit acknowledges the front.
class RecallEndQueue {
 std::map<std::pair<Time,std::uint64_t>,PendingRecallEnd> pending_;
 struct Watermark {unsigned effect=0;Time at=0;};
 std::map<Scope,Watermark> owners_;std::uint64_t sequence_=0;Time clock_=0;
public:
 bool enqueue(const RecallAnchor& anchor,Time at){
  if(!recall_scope(anchor.caster)||!recall_scope(anchor.target)||anchor.caster.instance!=anchor.target.instance||
     anchor.caster.actor==anchor.target.actor||!anchor.effect||at<anchor.applied||
     at<clock_||sequence_==UINT64_MAX||pending_.size()>=4096)return false;
  const auto previous=owners_.find(anchor.caster);
  if(previous==owners_.end()&&owners_.size()>=4096)return false;
  if(previous!=owners_.end()&&(anchor.effect<=previous->second.effect||at<previous->second.at))return false;
  // Frontiers are ordered globally. No late record may move behind an already
  // queued later event and invalidate its prepared snapshots.
  if(!pending_.empty()&&at<pending_.rbegin()->second.at)return false;
  const auto event=sequence_+1;
  pending_.emplace(std::pair{at,event},PendingRecallEnd{anchor,at,event});
  owners_.insert_or_assign(anchor.caster,Watermark{anchor.effect,at});sequence_=event;clock_=at;return true;
 }
 std::optional<PendingRecallEnd> front()const{
  return pending_.empty()?std::nullopt:std::optional(pending_.begin()->second);
 }
 bool acknowledge(std::uint64_t event){
  if(pending_.empty()||pending_.begin()->second.event!=event)return false;
  pending_.erase(pending_.begin());return true;
 }
 void retire(Scope caster){
  std::erase_if(pending_,[&](const auto& row){return row.second.anchor.caster==caster;});owners_.erase(caster);
 }
 std::size_t size()const{return pending_.size();}
};
}
