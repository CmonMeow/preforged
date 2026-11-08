#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace fw::gw::arena {
// One authoritative monotonic deadline per lobby. Retries and late arrivals
// never restart the clock. Membership is leased to a live world connection.
class Queue {
public:
 using Player=std::uint64_t;
 struct State { std::uint64_t deadline; unsigned players; unsigned remaining; };
 struct Match {std::uint64_t id; std::array<Player,2> players;};
 explicit Queue(std::uint64_t duration=30000):duration_(duration){if(!duration)throw std::invalid_argument("arena duration");}
 std::optional<State> join(std::uint64_t lobby,Player player,std::uint64_t now){
  if(!lobby||!player)return {};
  std::lock_guard lock(mutex_);
  for(const auto& [id,q]:queues_)if(id!=lobby&&(q.players[0]==player||q.players[1]==player))return {};
  auto it=queues_.find(lobby);
  if(it==queues_.end()){
   if(queues_.size()>=4096||now>UINT64_MAX-duration_)return {};
   it=queues_.emplace(lobby,Entry{now+duration_,{player,0}}).first;
  }else {
   auto& q=it->second;
   renew_solo(q,now);
   if(q.players[0]!=player&&q.players[1]!=player){
    if(now>=q.deadline||q.players[1])return {};
    q.players[1]=player;
   }
  }
  return state(it->second,now);
 }
 std::optional<State> inspect(std::uint64_t lobby,Player player,std::uint64_t now){
  std::lock_guard lock(mutex_);auto it=queues_.find(lobby);
  if(it==queues_.end()||(it->second.players[0]!=player&&it->second.players[1]!=player))return {};
  renew_solo(it->second,now);
  return state(it->second,now);
 }
 // Called by the server scheduler. A solo timeout starts the next window
 // without dropping membership; only cancellation removes a waiting player.
 std::optional<Match> expire(std::uint64_t lobby,std::uint64_t now){
  std::lock_guard lock(mutex_);auto it=queues_.find(lobby);
  if(it==queues_.end()||now<it->second.deadline)return {};
  const auto q=it->second;
  if(!q.players[1]){
   renew_solo(it->second,now);return {};
  }
  queues_.erase(it);
  return Match{++next_match_,q.players};
 }
 bool leave(std::uint64_t lobby,Player player){
  std::lock_guard lock(mutex_);auto it=queues_.find(lobby);if(it==queues_.end())return false;
  auto& q=it->second;
  if(q.players[0]==player){q.players[0]=q.players[1];q.players[1]=0;}
  else if(q.players[1]==player)q.players[1]=0;else return false;
  if(!q.players[0])queues_.erase(it);return true;
 }
private:
 struct Entry {std::uint64_t deadline;std::array<Player,2> players;};
 void renew_solo(Entry& q,std::uint64_t now)const{
  if(q.players[1]||now<q.deadline)return;
  // Admission and inspection can precede a delayed scheduler callback. Keep
  // the solo member in the current periodic window in all three paths.
  const auto intervals=(now-q.deadline)/duration_+1;
  if(intervals>(UINT64_MAX-q.deadline)/duration_)throw std::overflow_error("arena clock exhausted");
  q.deadline+=intervals*duration_;
 }
 static State state(const Entry& q,std::uint64_t now){auto ms=now<q.deadline?q.deadline-now:0;return {q.deadline,q.players[1]?2u:1u,static_cast<unsigned>(ms/1000+(ms%1000!=0))};}
 std::uint64_t duration_,next_match_=0;std::mutex mutex_;std::map<std::uint64_t,Entry> queues_;
};
}
