#pragma once
#include "gw_cooldown_ordering.h"
namespace fw::gw::skills::effects {
struct SkillbarTransition {Slots before{},after{};};
struct CooldownBarSnapshot {CooldownStamp published;std::uint64_t revision=0;Slots bar{};bool operator==(const CooldownBarSnapshot&)const=default;};
struct CooldownCompletion {CooldownStamp stamp;Slot slot;std::uint64_t skill_use=0,base_ms=0,existing_until=0;bool instant_recharge=false;std::uint64_t extra_ms=0;bool operator==(const CooldownCompletion&)const=default;};
// Retain the observed ordinary inputs even when this completion resets them.
// A reset is an ordered event, not an ordinary timer with a shorter deadline.
inline std::optional<std::uint64_t> completion_deadline(const CooldownCompletion& use){
 if(use.instant_recharge)return use.stamp.at;
 if(use.extra_ms>15000||use.base_ms>UINT64_MAX-use.extra_ms||use.base_ms+use.extra_ms>UINT64_MAX-use.stamp.at)return {};
 return std::max(use.existing_until,use.stamp.at+use.base_ms+use.extra_ms);
}
struct AttackDisableRequest {unsigned attack=0;std::uint64_t began=0,ends=0;};
struct CooldownControl {std::uint64_t epoch=0;std::optional<CooldownStamp> stamp;std::optional<Slot> slot;std::optional<AttackDisableRequest> attack_disable;};
enum class CooldownOriginKind {completion,control,restoration,amendment};
struct CooldownOrigin {CooldownStamp stamp;CooldownOriginKind kind=CooldownOriginKind::completion;std::uint64_t token=0;bool operator==(const CooldownOrigin&)const=default;};
struct LocalCooldownSource {Slot slot;std::uint64_t until=0;std::optional<CooldownOrigin> origin;bool operator==(const LocalCooldownSource&)const=default;};
struct LocalCooldownSnapshot {
 Scope owner;std::uint64_t revision=0,at=0;std::vector<LocalCooldownSource> timers;
 // Observations made while processing Runtime work, never polled just to
 // publish. World rejects a snapshot when newer source work is outstanding.
 std::uint64_t control_epoch=0,completion_order=0,reset_order=0,amendment_order=0;
 bool operator==(const LocalCooldownSnapshot&)const=default;
};
struct VerifiedCooldownSnapshot {LocalCooldownSnapshot snapshot;std::vector<Slot> verified;bool operator==(const VerifiedCooldownSnapshot&)const=default;};
struct CooldownCheckpointPermit {VerifiedCooldownSnapshot accepted;CooldownStamp watermark;};
// World-owned under its existing mutex. No callback into a session Runtime.
class CooldownBarAuthority {
 Scope owner_;CooldownOrderClock clock_;std::optional<CooldownBarSnapshot> bar_;
 struct Observation {unsigned effect;std::uint64_t ends;CooldownBarSnapshot bar;};
 std::optional<Observation> observation_;
public:
 explicit CooldownBarAuthority(Scope owner):owner_(owner),clock_(owner){}
 std::optional<CooldownBarSnapshot> publish(Scope owner,const Slots& bar,std::uint64_t now){
  if(owner!=owner_||(bar_&&now<bar_->published.at))return {};std::set<std::pair<unsigned,unsigned>> seen;
  for(const auto slot:bar){if(!slot.skill){if(slot.copy)return {};continue;}
   if(!cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
  }
  const bool changed=!bar_||bar_->bar!=bar;
  if(changed&&bar_&&bar_->revision==UINT64_MAX)return {};
  const auto stamp=clock_.next(owner,now);if(!stamp)return {};
  const auto revision=bar_?bar_->revision+(changed?1:0):1;
  bar_=CooldownBarSnapshot{*stamp,revision,bar};
  if(observation_&&now<observation_->ends)observation_->bar=*bar_;return bar_;
 }
 bool observe_bar_until(Scope owner,unsigned effect,std::uint64_t begins,std::uint64_t ends){
  if(owner!=owner_||!effect||ends<=begins||!bar_||bar_->published.at>begins||observation_)return false;
  observation_=Observation{effect,ends,*bar_};return true;
 }
 std::optional<CooldownBarSnapshot> observed_bar(Scope owner,unsigned effect,std::uint64_t ends)const{
  return owner==owner_&&observation_&&observation_->effect==effect&&observation_->ends==ends?std::optional(observation_->bar):std::nullopt;
 }
 bool release_bar_observation(Scope owner,unsigned effect){
  if(owner!=owner_||!observation_||observation_->effect!=effect)return false;observation_.reset();return true;
 }
 std::optional<CooldownBarSnapshot> snapshot(Scope owner)const{return owner==owner_?bar_:std::nullopt;}
 std::optional<CooldownStamp> order(Scope owner,std::uint64_t now){return clock_.next(owner,now);}
};
}
