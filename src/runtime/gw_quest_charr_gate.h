#pragma once
#include "gw_quest_state.h"
#include "gw_effect_registry.h"
#include <array>
#include <string_view>
#include <vector>
namespace fw::gw::quests::charr_gate {
inline constexpr unsigned id=46,experience=200;
using Scope=skills::effects::Scope;
inline constexpr std::array<std::uint64_t,4> enemy_credits{0x4602,0x4603,0x4604,0x4605};
struct Eligibility {bool prophecies=false,presearing=false;unsigned level=0;};
inline bool eligible(const Eligibility& e){return e.prophecies&&e.presearing&&e.level>=2;}
struct Interaction {Scope selected,current;std::string_view name;unsigned map=0;bool reachable=false;};
inline bool valid_scope(Scope s){return s.instance&&s.actor&&s.generation;}
inline bool matches(const Interaction& npc,unsigned map){return valid_scope(npc.selected)&&npc.selected==npc.current&&npc.name=="Prince Rurik"&&npc.map==map&&npc.reachable;}
inline bool valid_progress(const Progress& p){
 if(p.timed)return false;
 for(auto key:p.objective_credits)if(std::find(enemy_credits.begin(),enemy_credits.end(),key)==enemy_credits.end())return false;
 if(p.stage==Stage::completed||p.stage==Stage::rewarded)return p.objective_credits.size()==4;
 if(p.stage==Stage::active)return p.objective_credits.size()<4;
 return p.stage==Stage::abandoned;
}
struct RemainingEnemy {unsigned index;std::uint64_t credit;};
// Reconnect must select the exact uncredited logical enemies. A count alone
// loses identity when the completed kill was not the first template.
inline std::optional<std::vector<RemainingEnemy>> remaining_enemies(const Progress& progress){
 if(progress.stage!=Stage::active||!valid_progress(progress))return {};
 std::vector<RemainingEnemy> remaining;remaining.reserve(enemy_credits.size()-progress.objective_credits.size());
 for(unsigned i=0;i<enemy_credits.size();++i)
  if(!progress.objective_credits.contains(enemy_credits[i]))remaining.push_back({i,enemy_credits[i]});
 return remaining;
}
inline std::optional<Profile> accept(const Profile& current,unsigned cycle,const Eligibility& e,const Interaction& npc){
 if(!eligible(e)||!matches(npc,148))return {};
 const auto next=quests::accept(current,id);if(!next||next->records.at(id).cycle!=cycle)return {};return next;
}
// Trusted encounter-start boundary only: bind exact current server-owned lifetimes for
// one quest attempt. No network/UI message is encounter-start authorization or a death event.
// Optional empty enemy scopes are permitted ONLY for already durable credits,
// so reconnect does not respawn killed objective actors.
class Attempt {
 Scope owner_,escort_;unsigned cycle_=0;std::array<Scope,4> enemies_{};
 Attempt(Scope owner,Scope escort,unsigned cycle,std::array<Scope,4> enemies):owner_(owner),escort_(escort),cycle_(cycle),enemies_(enemies){}
 bool owns(const Profile& current,Scope owner)const{
  const auto found=current.records.find(id);return owner==owner_&&found!=current.records.end()&&
   found->second.cycle==cycle_&&found->second.stage==Stage::active&&valid_progress(found->second);
 }
public:
 static std::optional<Attempt> bind(const Profile& current,Scope owner,unsigned cycle,unsigned map,Scope escort,Scope current_escort,std::array<Scope,4> enemies){
  validate(current);const auto found=current.records.find(id);
  if(map!=146||!valid_scope(owner)||!valid_scope(escort)||escort!=current_escort||owner.instance!=escort.instance||owner==escort||
     found==current.records.end()||found->second.cycle!=cycle||found->second.stage!=Stage::active||!valid_progress(found->second))return {};
  std::set<std::uint32_t> seen{owner.actor,escort.actor};
  if(seen.size()!=2)return {};
  for(unsigned i=0;i<4;++i){const auto scope=enemies[i];
   if(found->second.objective_credits.contains(enemy_credits[i])){if(scope!=Scope{})return {};continue;}
   if(!valid_scope(scope)||scope.instance!=owner.instance||!seen.insert(scope.actor).second)return {};
  }
  return Attempt(owner,escort,cycle,enemies);
 }
 // Called after authoritative death, including an approved friendly NPC kill.
 // The exact lifetime, not the killer or bare client agent number, grants credit.
 std::optional<Profile> death(const Profile& current,Scope owner,Scope dead)const{
  if(!owns(current,owner)||!valid_scope(dead))return {};
  for(unsigned i=0;i<4;++i)if(dead==enemies_[i])return credit_objective(current,id,cycle_,enemy_credits[i],4);
  return {};
 }
};
}

