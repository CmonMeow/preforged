#pragma once
#include "gw_effect_registry.h"
#include <map>
#include <stdexcept>
#include <vector>
namespace fw::gw::skills::effects {
// Each plan function gives a distinct timer/link type. Early cancellation
// suppresses natural-expiry work; consumed deadlines cannot be replayed.
template<auto Plan>
class NaturalHexTimers {
public:
 struct Link {
  Scope caster,target;unsigned effect=0,rank=0,level=0;Time applied=0,expires=0;
  auto operator<=>(const Link&)const=default;
 };
private:
 std::uint64_t instance_;Time clock_=0;
 std::map<Scope,Link> links_;
 std::map<std::pair<Time,Scope>,Scope> deadlines_;
 bool valid_scope(Scope s)const{return s.instance==instance_&&s.actor&&s.actor<=65535&&s.generation;}
 bool valid(const Link& link)const{
  const auto plan=Plan(link.rank);
  return plan&&valid_scope(link.caster)&&valid_scope(link.target)&&link.caster!=link.target&&link.effect&&
   link.level&&link.level<=255&&link.applied>=clock_&&link.applied<=UINT64_MAX-plan->duration&&
   link.expires==link.applied+plan->duration;
 }
public:
 explicit NaturalHexTimers(std::uint64_t instance):instance_(instance){
  if(!instance)throw std::invalid_argument("Natural hex instance required");
 }
 bool add(const Link& link){
  if(!valid(link)||links_.size()>=4096||links_.contains(link.target))return false;
  const auto [it,inserted]=links_.emplace(link.target,link);if(!inserted)return false;
  try{deadlines_.emplace(std::pair{link.expires,link.target},link.target);}
  catch(...){links_.erase(it);throw;}clock_=link.applied;return true;
 }
 bool replace(Scope target,unsigned previous,const Link& replacement,Time now){
  const auto old=links_.find(target);
  if(old==links_.end()||old->second.effect!=previous||now<clock_||now<old->second.applied||now>=old->second.expires||
     replacement.target!=target||replacement.applied!=now||replacement.effect==previous||!valid(replacement))return false;
  // Allocate the new deadline first. Failure preserves the prior obligation.
  const auto key=std::pair{replacement.expires,target};
  const auto old_key=std::pair{old->second.expires,target};
  if(key!=old_key)deadlines_.emplace(key,target);
  old->second=replacement;if(key!=old_key)deadlines_.erase(old_key);clock_=now;return true;
 }
 bool cancel(Scope target,unsigned effect,Time now){
  const auto old=links_.find(target);
  if(old==links_.end()||old->second.effect!=effect||now<clock_||now<old->second.applied||now>=old->second.expires)return false;
  deadlines_.erase({old->second.expires,target});links_.erase(old);clock_=now;return true;
 }
 std::optional<Time> next_deadline()const{
  return deadlines_.empty()?std::nullopt:std::optional(deadlines_.begin()->first.first);
 }
 std::vector<Link> advance(Time now){
  if(now<clock_)return {};clock_=now;std::vector<Link> result;
  while(!deadlines_.empty()&&deadlines_.begin()->first.first<=now){
   const auto target=deadlines_.begin()->second;deadlines_.erase(deadlines_.begin());
   const auto found=links_.find(target);result.push_back(found->second);links_.erase(found);
  }
  // Obligations are consumed before any world callback can reenter dispatch.
  return result;
 }
 std::size_t size()const{return links_.size();}
 std::optional<Link> lookup(Scope target)const{
  const auto found=links_.find(target);return found==links_.end()?std::nullopt:std::optional(found->second);
 }
 void discard(Scope actor){
  for(auto it=links_.begin();it!=links_.end();)if(it->second.caster==actor||it->second.target==actor){
   deadlines_.erase({it->second.expires,it->first});it=links_.erase(it);
  }else ++it;
 }
};
}
