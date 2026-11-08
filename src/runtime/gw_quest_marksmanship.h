#pragma once
#include "gw_effect_registry.h"
#include <array>
#include <optional>
#include <set>
#include <span>
namespace fw::gw::quests {
// Quest42: wiki/capture90,000ms, five logical worms. This planner does not
// authorize NPC dialogue, publish clocks, save progress or award rewards.
class MarksmanshipObjective {
public:
 using Scope=skills::effects::Scope;
 enum class State {active,completed,failed};
 static constexpr std::uint64_t duration_ms=90000;
private:
 Scope owner_{};std::uint32_t cycle_=0;std::uint64_t start_=0,deadline_=0;
 std::array<Scope,5> actors_{};std::array<bool,5> credited_{};
 std::set<Scope> used_;State state_=State::active;
 bool window(std::uint64_t now)const{return now>=start_&&now<deadline_;}
 bool owned(Scope owner,std::uint32_t cycle)const{return owner==owner_&&cycle==cycle_;}
public:
 static std::optional<MarksmanshipObjective> create(Scope owner,std::uint32_t cycle,
        std::uint64_t start,std::span<const Scope> actors){
  if(!owner.instance||!owner.actor||!owner.generation||!cycle||actors.size()!=5||start>UINT64_MAX-duration_ms)return {};
  MarksmanshipObjective result;result.owner_=owner;result.cycle_=cycle;result.start_=start;result.deadline_=start+duration_ms;
  std::set<std::uint32_t> identities;
  for(std::size_t i=0;i<actors.size();++i){const auto actor=actors[i];
   if(actor.instance!=owner.instance||!actor.actor||!actor.generation||actor.actor==owner.actor||
      !identities.insert(actor.actor).second||!result.used_.insert(actor).second)return {};
   result.actors_[i]=actor;
  }
  return result;
 }
 // Trusted persistence supplies elapsed wall-clock time and the five logical
 // credit bits. A fresh instance gets new actor lifetimes but no fresh timer.
 static std::optional<MarksmanshipObjective> restore(Scope owner,std::uint32_t cycle,
        std::uint64_t now,std::uint64_t elapsed,unsigned credited_mask,std::span<const Scope> actors){
  if(elapsed>duration_ms||credited_mask>31||now>UINT64_MAX-(duration_ms-elapsed))return {};
  // create's full-duration overflow guard is intentionally independent of
  // restore: near UINT64_MAX the shortened remaining window can still fit.
  auto result=create(owner,cycle,0,actors);if(!result)return {};
  result->start_=now;result->deadline_=now+duration_ms-elapsed;
  for(unsigned i=0;i<5;++i)result->credited_[i]=(credited_mask&(1u<<i))!=0;
  if(!result->remaining())result->state_=State::completed;
  else if(elapsed==duration_ms)result->state_=State::failed;
  return result;
 }
 std::optional<unsigned> logical_credit(Scope actor)const{
  for(unsigned i=0;i<5;++i)if(actors_[i]==actor)return i+1;
  return {};
 }
 bool death(Scope owner,std::uint32_t cycle,Scope actor,std::uint64_t occurred_at){
  if(!owned(owner,cycle)||state_!=State::active||!window(occurred_at))return false;
  for(std::size_t i=0;i<actors_.size();++i)if(actors_[i]==actor&&!credited_[i]){
   credited_[i]=true;if(remaining()==0)state_=State::completed;return true;
  }
  return false;
 }
 // Trusted world caller must establish that this is the same logical worm,
 // e.g. a burrow lifetime replacement; recycled IDs alone do not authorize it.
 bool replace_actor(Scope owner,std::uint32_t cycle,Scope old_actor,Scope next,std::uint64_t now){
  if(!owned(owner,cycle)||state_!=State::active||!window(now)||next.instance!=owner_.instance||
     !next.actor||!next.generation||next.actor==owner_.actor||used_.contains(next))return false;
  for(const auto& actor:actors_)if(actor!=old_actor&&actor.actor==next.actor)return false;
  for(std::size_t i=0;i<actors_.size();++i)if(actors_[i]==old_actor&&!credited_[i]){
   used_.insert(next);actors_[i]=next;return true;
  }
  return false;
 }
 bool expire(Scope owner,std::uint32_t cycle,std::uint64_t now){
  if(!owned(owner,cycle)||state_!=State::active||now<deadline_)return false;
  state_=State::failed;return true;
 }
 unsigned remaining()const{unsigned count=0;for(bool credited:credited_)if(!credited)++count;return count;}
 std::uint64_t deadline()const{return deadline_;}
 State state()const{return state_;}
};
}
