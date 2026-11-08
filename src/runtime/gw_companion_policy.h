#pragma once
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

namespace fw::gw::ai::companion {
// Policy only: no packet emission, pathfinding, target ranking, skill selection,
// resource mutation, or assumptions about normalized allegiance/model IDs.
struct Actor {
 std::uint64_t instance=0,generation=0;
 std::uint32_t id=0;
 bool operator==(const Actor&)const=default;
 bool valid()const{return instance&&generation&&id;}
};
enum class Profile {friendly_follower,ranger_pet,minion,summoned_ally};
enum class PetMode {guard,heel,attack};
enum class Intent {idle,follow_owner,pursue_target,engage_target,unsupported_configuration};
enum class Reason {none,dead,invalid_owner,invalid_actor,missing_distance,invalid_configuration,
 follow_distance,combat_recall,pet_heel,pet_lock,retained_target,acquired_target,no_target};
enum class Trigger {owner_attack,adjacent_owner_threat,proximity,assisted_combat};
struct Config {
 // No retail thresholds are inferred. The caller supplies reviewed values.
 std::optional<double> follow_start,follow_stop;
 std::optional<double> follower_combat_break;
 std::optional<double> pet_attack_lock_range;
 std::optional<double> pet_guard_combat_break;
};
struct Target {
 Actor actor;
 bool alive=false,hostile=false;
 // Caller checks current action range, LoS, plane and other execution gates.
 bool action_ready=false;
 double distance=0;
};
struct Acquisition {
 Actor target;
 Trigger trigger=Trigger::owner_attack;
 // Caller certifies proximity/owner-target acquisition conditions, using
 // reviewed profile configuration. False never acquires a target.
 bool permitted=false;
};
struct Snapshot {
 Actor self,owner;
 bool alive=false;
 std::optional<double> owner_distance;
 std::vector<Target> targets;
 std::optional<Acquisition> acquisition;
 // Pending windup/cast only. Released projectiles are separate world objects.
 std::optional<Actor> pending_action_target;
};
struct PetCommand {PetMode mode=PetMode::guard;std::optional<Actor> target;};
struct Decision {
 Intent intent=Intent::idle;
 Reason reason=Reason::none;
 std::optional<Actor> target;
 bool cancel_pending_action=false;
};
class Controller {
 Profile profile_;
 Actor self_,owner_;
 std::optional<Actor> target_;
 PetMode pet_mode_=PetMode::guard,return_mode_=PetMode::guard;
 bool following_=false;
 bool binding_changed_=false;
 static bool distance(double x){return std::isfinite(x)&&x>=0;}
 static bool valid_config(const Config& c){
  for(auto value:{c.follow_start,c.follow_stop,c.follower_combat_break,c.pet_attack_lock_range,c.pet_guard_combat_break})
   if(value&&!distance(*value))return false;
  return !c.follow_start||!c.follow_stop||*c.follow_stop<=*c.follow_start;
 }
 const Target* resolve(const Snapshot& s,Actor key)const{
  if(!key.valid()||key.instance!=s.self.instance||key==s.self||key==s.owner)return nullptr;
  const Target* found=nullptr;
  for(const auto& t:s.targets)if(t.actor==key){if(found)return nullptr;found=&t;}
  return found&&found->alive&&found->hostile&&distance(found->distance)?found:nullptr;
 }
 void clear(){target_.reset();following_=false;pet_mode_=return_mode_=PetMode::guard;}
 Decision finish(const Snapshot& s,Intent intent,Reason reason){
  const bool combat=intent==Intent::engage_target||intent==Intent::pursue_target;
  const auto target=combat?target_:std::nullopt;
  return {intent,reason,target,s.pending_action_target&&
   (binding_changed_||intent!=Intent::engage_target||target!=s.pending_action_target)};
 }
 Decision follow(const Snapshot& s,const Config& c,Reason reason,bool force=false){
  target_.reset();
  if(!c.follow_start||!c.follow_stop)return finish(s,Intent::unsupported_configuration,Reason::invalid_configuration);
  if(!s.owner_distance||!distance(*s.owner_distance))return finish(s,Intent::unsupported_configuration,Reason::missing_distance);
  if(*s.owner_distance<=*c.follow_stop)following_=false;
  else if(force||*s.owner_distance>*c.follow_start)following_=true;
  return finish(s,following_?Intent::follow_owner:Intent::idle,reason);
 }
 bool permits(Trigger trigger)const{
  switch(profile_){
  case Profile::ranger_pet:return trigger==Trigger::owner_attack||trigger==Trigger::adjacent_owner_threat;
  case Profile::minion:case Profile::summoned_ally:return trigger==Trigger::owner_attack||trigger==Trigger::proximity;
  case Profile::friendly_follower:return trigger==Trigger::owner_attack||trigger==Trigger::assisted_combat;
  }
  return false;
 }
public:
 explicit Controller(Profile profile):profile_(profile){}
 PetMode pet_mode()const{return pet_mode_;}
 std::optional<Actor> target()const{return target_;}
 // Commands are processed with the current authoritative snapshot so a reused
 // ID or cross-instance target cannot become a new lock. Invalid command leaves
 // state unchanged. Guard/Heel clear an existing lock/engagement immediately.
 bool command(const PetCommand& cmd,const Snapshot& s){
  if(profile_!=Profile::ranger_pet||s.self!=self_||s.owner!=owner_||!s.alive)return false;
  if(cmd.mode==PetMode::attack){
   if(!cmd.target||!resolve(s,*cmd.target))return false;
   if(pet_mode_!=PetMode::attack)return_mode_=pet_mode_;
   pet_mode_=PetMode::attack;target_=cmd.target;following_=false;
  }else{pet_mode_=return_mode_=cmd.mode;target_.reset();}
  return true;
 }
 Decision tick(const Snapshot& s,const Config& c){
  binding_changed_=self_!=s.self||owner_!=s.owner;
  if(binding_changed_){clear();self_=s.self;owner_=s.owner;}
  if(!s.self.valid()){clear();return finish(s,Intent::idle,Reason::invalid_actor);}
  if(!s.alive){clear();return finish(s,Intent::idle,Reason::dead);}
  if(!s.owner.valid()||s.owner.instance!=s.self.instance||s.owner==s.self){clear();return finish(s,Intent::idle,Reason::invalid_owner);}
  if(!valid_config(c)){target_.reset();return finish(s,Intent::unsupported_configuration,Reason::invalid_configuration);}
  const Target* current=target_?resolve(s,*target_):nullptr;
  if(!current)target_.reset();
  if(profile_==Profile::ranger_pet&&pet_mode_==PetMode::attack){
   if(!c.pet_attack_lock_range)return finish(s,Intent::unsupported_configuration,Reason::invalid_configuration);
   if(!current||current->distance>*c.pet_attack_lock_range){
    target_.reset();current=nullptr;pet_mode_=return_mode_;
   }
  }
  if(profile_==Profile::ranger_pet&&pet_mode_==PetMode::heel)return follow(s,c,Reason::pet_heel,true);
  // Party followers prioritize configured owner recall over their engagement.
  // Minions/summons deliberately do not consult this party-follow threshold.
  if(profile_==Profile::friendly_follower||(profile_==Profile::ranger_pet&&pet_mode_==PetMode::guard)){
   const auto recall=profile_==Profile::friendly_follower?c.follower_combat_break:c.pet_guard_combat_break;
   if(!recall)return finish(s,Intent::unsupported_configuration,Reason::invalid_configuration);
   if(!s.owner_distance||!distance(*s.owner_distance))return finish(s,Intent::unsupported_configuration,Reason::missing_distance);
   if(*s.owner_distance>*recall)return follow(s,c,Reason::combat_recall,true);
   if(following_&&c.follow_stop&&*s.owner_distance>*c.follow_stop)return follow(s,c,Reason::follow_distance);
  }
  Reason reason=Reason::retained_target;
  // A pet guarding its owner may change to the owner's newly chosen target.
  // Locked pets and committed summons/minions keep their live current target.
  const bool can_acquire=!current||(profile_==Profile::ranger_pet&&pet_mode_==PetMode::guard);
  if(can_acquire&&s.acquisition&&s.acquisition->permitted&&permits(s.acquisition->trigger)){
   if(const auto* candidate=resolve(s,s.acquisition->target)){
    target_=candidate->actor;current=candidate;following_=false;reason=Reason::acquired_target;
   }
  }
  if(current){
   if(profile_==Profile::ranger_pet&&pet_mode_==PetMode::attack)reason=Reason::pet_lock;
   return finish(s,current->action_ready?Intent::engage_target:Intent::pursue_target,reason);
  }
  return follow(s,c,Reason::no_target);
 }
};
} // namespace fw::gw::ai::companion
