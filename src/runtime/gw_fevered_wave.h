#pragma once
#include "gw_fevered_dreams.h"
#include <deque>
#include <map>
namespace fw::gw::skills::effects {
// Immutable World snapshot input. Foe relationships are relative to each
// original hex caster, not the actor receiving the incoming condition.
struct FeveredNode {
 Scope scope;Registry registry;combat::conditions::State conditions;
 FeveredEvents events;std::optional<FeveredOwnership> hex;
 double x=0,y=0;bool alive=true;
};
using FeveredRelations=std::map<std::pair<Scope,Scope>,bool>;
struct FeveredCommit {Scope target;combat::conditions::State before,after;FeveredConditionEvent event;};
struct FeveredWave {std::vector<FeveredNode> nodes;std::vector<FeveredCommit> commits;std::uint64_t next_event=0;};
// All-or-nothing proposal only: World must settle HP/Deep Wound/Fragility and
// validate lifetimes before publication. Queue ordering is explicit FIFO;
// retail simultaneous-reaction ordering is not inferred from activation logs.
inline std::optional<FeveredWave> prepare_fevered_wave(
 const std::vector<FeveredNode>& input,const FeveredRelations& relations,
 Scope root,const combat::conditions::State& root_after,Time now,
 std::uint64_t next_event,FeveredDazedSpan span,bool hex_application=false){
 if(input.empty()||!next_event||(span!=FeveredDazedSpan::attempted_duration&&span!=FeveredDazedSpan::retained_remaining))return {};
 FeveredWave result{input,{},next_event};std::map<Scope,std::size_t> indexes;std::set<unsigned> actors;
 for(std::size_t i=0;i<input.size();++i){const auto& n=input[i];
  if(n.scope.instance!=root.instance||!n.scope.actor||n.scope.actor>65535||!n.scope.generation||!n.registry.matches(n.scope)||!std::isfinite(n.x)||!std::isfinite(n.y)||!actors.insert(n.scope.actor).second)return {};
  indexes.emplace(n.scope,i);
 }
 const auto start=indexes.find(root);if(start==indexes.end()||!input[start->second].alive)return {};
 for(const auto& n:input)if(n.hex){const auto c=indexes.find(n.hex->caster);
  if(n.hex->target!=n.scope||c==indexes.end()||!input[c->second].alive||!owned_fevered_dreams(n.registry,*n.hex,now))return {};
 }
 std::deque<FeveredReaction> queue;
 auto commit=[&](std::size_t index,const combat::conditions::State& after,bool application)->bool {
  if(result.next_event==UINT64_MAX)return false;
  auto& node=result.nodes[index];const auto before=node.conditions;
  const FeveredConditionEvent event{node.scope,result.next_event++,now,true};
  if(!node.events.accept(event))return false;
  node.conditions=after;
  if(node.hex){auto reaction=fevered_condition_reaction(node.registry,*node.hex,event,before,after,span,application);
   if(!reaction)return false;node.conditions=reaction->target;
   if(!reaction->pulses.empty())queue.push_back(std::move(*reaction));
  }
  result.commits.push_back({node.scope,before,node.conditions,event});return true;
 };
 if(!commit(start->second,root_after,hex_application))return {};
 // Every queued reaction after the root requires a newly gained condition.
 // No removals occur at fixed time: ten kinds per actor bound the queue.
 const auto budget=input.size()>((SIZE_MAX-1)/10)?SIZE_MAX:1+input.size()*10;
 std::size_t processed=0;
 while(!queue.empty()){
  if(++processed>budget)return {};
  auto reaction=std::move(queue.front());queue.pop_front();const auto& node=result.nodes[indexes.at(reaction.owner.target)];
  std::vector<FeveredContact> contacts;
  for(const auto& other:result.nodes){
   if(!other.alive||other.scope==node.scope||other.scope==reaction.owner.caster)continue;
   const double distance=std::hypot(other.x-node.x,other.y-node.y);if(distance>322)continue;
   const auto relation=relations.find({reaction.owner.caster,other.scope});if(relation==relations.end())return {};
   contacts.push_back({other.scope,static_cast<float>(distance),true,relation->second,other.conditions});
  }
  auto recipients=fevered_recipients(reaction,contacts);if(!recipients)return {};
  for(const auto& recipient:*recipients)
   if(!commit(indexes.at(recipient.owner),recipient.after,false))return {};
 }

 return result;
}
}
