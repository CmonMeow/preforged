#pragma once
#include "gw_spiteful_spirit.h"
#include "gw_effect_registry.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace fw::gw::skills::effects {
enum class SpitefulPhase {attack_release,skill_completed};
enum class SpitefulTrigger {invalid,none,trigger};
// Caller supplies a server-owned release or successful use-completion event.
// This classifier does not authenticate or deduplicate raw network messages.
inline SpitefulTrigger spiteful_trigger(SpitefulPhase phase,unsigned skill){
 if(phase==SpitefulPhase::attack_release){
  if(!skill)return SpitefulTrigger::trigger;
  const auto* row=find_skill(skill);
  return row&&row->type==14?SpitefulTrigger::trigger:SpitefulTrigger::invalid;
 }
 if(phase!=SpitefulPhase::skill_completed)return SpitefulTrigger::invalid;
 const auto* row=find_skill(skill);if(!row||!skill)return SpitefulTrigger::invalid;
 return row->type==14?SpitefulTrigger::none:SpitefulTrigger::trigger;
}
struct SpitefulAction {
 Scope owner;SpitefulPhase phase=SpitefulPhase::attack_release;
 unsigned skill=0;std::uint64_t event=0,at=0;bool success=true;
};
// Producer IDs must be monotonic per observed phase,not IDs reserved when an
// earlier long cast began. Attack release and completed-use IDs are separate
// domains. IDs represent logical actions,not one ID per area recipient.
// World owns their authorization and chooses the reaction phase.
class SpitefulEvents {
 Scope owner_;std::uint64_t attack_=0,skill_=0,clock_=0;
public:
 struct Result {std::optional<SpitefulSpiritPlan> reaction;std::optional<Entry> hex;};
 explicit SpitefulEvents(Scope owner):owner_(owner){}
 std::optional<Result> observe(const SpitefulAction& action,std::optional<Entry> current){
  if(action.owner!=owner_||!owner_.instance||!owner_.actor||owner_.actor>65535||!owner_.generation||
     !action.event||action.at<clock_)return {};
  const auto trigger=spiteful_trigger(action.phase,action.skill);if(trigger==SpitefulTrigger::invalid)return {};
  const auto last=action.phase==SpitefulPhase::attack_release?attack_:skill_;
  if(action.event<=last)return {};
  Result result;
  if(current){
   const auto plan=spiteful_spirit(current->rank);
   if(current->skill!=121||current->kind!=Category::hex||!current->instance||!current->cause||
      current->cause>65535||current->cause==owner_.actor||!current->expires||*current->expires<=current->applied||!plan)return {};
   if(action.success&&trigger==SpitefulTrigger::trigger&&action.at>=current->applied&&action.at<*current->expires){result.reaction=plan;result.hex=current;}
  }
  if(action.phase==SpitefulPhase::attack_release)attack_=action.event;else skill_=action.event;
  clock_=action.at;return result;
 }
};
struct SpitefulAreaActor {
 Scope scope;float x=0,y=0;unsigned plane=0;bool alive=false;
 std::optional<bool> ally_to_center; // Pairwise fact for this snapshot center,not a guild/faction ID.
};
// Reviewed raw NPC team tokens,not the observer-relative allegiance byte.
// Native78fa40/58be00: equal tokens classify by definition flags; different
// regular tokens are enemies. Noncombat/animal categories stay unresolved.
inline std::optional<bool> spiteful_npc_alliance(std::optional<std::uint32_t> a,unsigned a_flags,
        std::optional<std::uint32_t> b,unsigned b_flags){
 const auto special=[](std::uint32_t token){return token==0x616e696d||token==0x6e6f6e63||token==0x6e6f6e6e;};
 const auto noncombat=[](unsigned flags){return !(flags&0x30)&&(flags&0x200);};
 if(!a||!b||special(*a)||special(*b)||noncombat(a_flags)||noncombat(b_flags))return {};
 return *a==*b;
}
enum class SpitefulContact {invalid,no_contact,unknown,eligible};
inline bool valid_spiteful_actor(const SpitefulAreaActor& a){
 return a.scope.instance&&a.scope.actor&&a.scope.actor<=65535&&a.scope.generation&&
  a.plane<=65535&&std::isfinite(a.x)&&std::isfinite(a.y);
}
inline SpitefulContact spiteful_contact(const SpitefulSpiritPlan& plan,
 const SpitefulAreaActor& center,const SpitefulAreaActor& candidate){
 const auto canonical=spiteful_spirit(plan.rank);
 if(!canonical||*canonical!=plan||!valid_spiteful_actor(center)||!valid_spiteful_actor(candidate))return SpitefulContact::invalid;
 if(center.scope.instance!=candidate.scope.instance||!center.alive||!candidate.alive||center.plane!=candidate.plane||
    std::hypot(double(center.x)-candidate.x,double(center.y)-candidate.y)>plan.radius)return SpitefulContact::no_contact;
 if(center.scope.actor==candidate.scope.actor)return center.scope==candidate.scope?SpitefulContact::eligible:SpitefulContact::invalid;
 if(!candidate.ally_to_center)return SpitefulContact::unknown;
 return *candidate.ally_to_center?SpitefulContact::eligible:SpitefulContact::no_contact;
}
struct SpitefulArea {
 std::vector<Scope> recipients;unsigned unknown=0;
 bool complete()const{return unknown==0;}
};
// World supplies every nearby actor,including actors with unknown alliance.
// Immutable snapshot,not a damage commit. World must refuse a partial area when
// alliance facts are unknown and preflight Health/reactions for every recipient.
inline std::optional<SpitefulArea> spiteful_area(const SpitefulSpiritPlan& plan,
 Scope center_scope,const std::vector<SpitefulAreaActor>& population){
 if(population.empty()||population.size()>4096)return {}; // Defensive cap,not a retail actor limit.
 std::set<std::pair<std::uint64_t,unsigned>> slots;const SpitefulAreaActor* center=nullptr;
 for(const auto& actor:population){
  if(!valid_spiteful_actor(actor)||!slots.emplace(actor.scope.instance,actor.scope.actor).second)return {};
  if(actor.scope==center_scope)center=&actor;
 }
 if(!center||!center->alive)return {};
 SpitefulArea result;
 for(const auto& actor:population)switch(spiteful_contact(plan,*center,actor)){
  case SpitefulContact::invalid:return {};
  case SpitefulContact::unknown:++result.unknown;break;
  case SpitefulContact::eligible:result.recipients.push_back(actor.scope);break;
  case SpitefulContact::no_contact:break;
 }
 std::sort(result.recipients.begin(),result.recipients.end());return result;
}
}
