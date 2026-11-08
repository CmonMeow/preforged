#pragma once
#include "gw_effect_registry.h"
#include "gw_attack_interrupts.h"
#include <map>
#include <limits>
namespace fw::gw::skills::effects {
struct GlimmeringMarkPlan {
 unsigned rank=0,damage=5,blind_ms=3000,pulses=10;
 Time interval=1000,duration=10000;
 float range=1248,adjacent=166;
 bool operator==(const GlimmeringMarkPlan&)const=default;
};
inline std::optional<GlimmeringMarkPlan> glimmering_mark(unsigned rank){
 const auto* r=find_skill(227);
 if(rank>20||!r||r->type!=4||r->profession!=6||r->attribute!=8||!r->elite()||
    r->energy_cost()!=10||r->activation!=1||r->aftercast!=.75f||r->recharge!=15||
    r->scale0!=5||r->scale15!=25||r->bonus0!=3||r->bonus15!=3||
    r->duration0!=10||r->duration15!=10||r->target_body_resource!=400)return {};
 return GlimmeringMarkPlan{rank,interpolate(5,25,rank)};
}
// World supplies the currently activating action, not merely an equipped bar.
// Ordinary attacks do not qualify. Unknown/malformed actions cannot silently
// be treated as ordinary attacks or as a proven attack skill.
inline std::optional<bool> glimmering_mark_blinds(const combat::weapon::AttackActionFacts& action){
 const auto classified=combat::weapon::classify_interrupt_action(action);
 if(!classified)return {};
 if(action.skill){const auto* row=find_skill(action.skill->skill);if(!row||!row->type)return {};}
 return classified->active&&action.skill&&find_skill(action.skill->skill)->type==14;
}
struct GlimmeringMarkPulse {
 Scope caster,target;unsigned effect=0,level=0;GlimmeringMarkPlan plan;
 Time applied=0,at=0;unsigned ordinal=0;
};
struct GlimmeringTargetedUse {
 Scope caster,target;unsigned skill=0;std::uint64_t event=0;Time at=0;
};
inline bool glimmering_direct_target_skill(unsigned skill){
 const auto* row=find_skill(skill);
 // Pinned foe and projectile/location-on-foe targeting. Caster-centred spells
 // such as Shockwave937 have target0 and cannot end an unrelated selected mark.
 return row&&row->id&&row->type&&(row->target==5||row->target==16);
}
// Owned moving-target timeline only. World must prove registry ownership,
// current target position/plane, hostility and every pulse's recipient cohort.
// No static ground center or remembered actor-ID-only recipients are stored.
class GlimmeringMarkSchedule {
 using Key=std::pair<Time,std::uint64_t>;
 std::map<Key,GlimmeringMarkPulse> pending_;
 std::map<Scope,Key> targets_;
 std::uint64_t next_=1;Time clock_=0;bool advancing_=false;
 static bool valid_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
 bool settled(Time now)const{return now>=clock_&&(!next_deadline()||*next_deadline()>now)&&(!advancing_||now==clock_);}
public:
 std::size_t size()const{return pending_.size();}
 std::optional<Time> next_deadline()const{return pending_.empty()?std::nullopt:std::optional(pending_.begin()->first.first);}
 bool add(Scope caster,Scope target,unsigned effect,unsigned level,const GlimmeringMarkPlan& plan,Time now){
  const auto canonical=glimmering_mark(plan.rank);
  if(!canonical||*canonical!=plan||!valid_scope(caster)||!valid_scope(target)||caster.instance!=target.instance||
     caster.actor==target.actor||!effect||!level||level>255||!settled(now)||next_==UINT64_MAX||
     now>UINT64_MAX-plan.duration-plan.blind_ms||(!targets_.contains(target)&&size()>=4096))return false;
  if(const auto old=targets_.find(target);old!=targets_.end()){pending_.erase(old->second);targets_.erase(old);}
  const Key key{now+plan.interval,next_++};
  pending_.emplace(key,GlimmeringMarkPulse{caster,target,effect,level,plan,now,key.first,1});targets_.emplace(target,key);
  clock_=now;return true;
 }
 // A proved committed targeted skill use, never autoattack or an area splash.
 // Caller settles earlier/equal pulses first. Activation/finish frontier choice
 // still requires World integration and retail evidence.
 std::optional<bool> targeted_use_completed(Scope caster,Scope selected,unsigned skill,Time now){
  const auto* used=find_skill(skill);
  if(!valid_scope(caster)||!valid_scope(selected)||caster.instance!=selected.instance||!skill||!used||!used->type||!settled(now))return {};
  const auto found=targets_.find(selected);clock_=now;
  if(found==targets_.end()||pending_.at(found->second).caster!=caster)return false;
  pending_.erase(found->second);targets_.erase(found);return true;
 }
 // Exact lifetime retirement. World owns settlement to the retirement boundary.
 bool end_target(Scope target,unsigned effect){
  const auto found=targets_.find(target);
  if(found==targets_.end()||pending_.at(found->second).effect!=effect)return false;
  pending_.erase(found->second);targets_.erase(found);return true;
 }
 void retire(Scope scope){
  for(auto it=pending_.begin();it!=pending_.end();)if(it->second.caster==scope||it->second.target==scope){targets_.erase(it->second.target);it=pending_.erase(it);}else ++it;
 }
 template<class Apply>bool advance(Time now,Apply&& apply){
  if(advancing_||now<clock_)return false;
  struct Reset{bool& active;~Reset(){active=false;}} reset{advancing_};advancing_=true;
  unsigned work=0;
  while(!pending_.empty()&&pending_.begin()->first.first<=now){
   if(work++==65536)return false; // resumable bound; no consumed pulse replays
   auto it=pending_.begin();const auto order=it->first.second;auto pulse=it->second;
   pending_.erase(it);targets_.erase(pulse.target);clock_=pulse.at;
   if(pulse.ordinal<pulse.plan.pulses){auto next=pulse;next.at+=next.plan.interval;++next.ordinal;
    const Key key{next.at,order};pending_.emplace(key,next);targets_[next.target]=key;}
   // Consume before callbacks. A callback may retire or renew this exact mark.
   if(!apply(pulse)){const auto remaining=targets_.find(pulse.target);
    if(remaining!=targets_.end()&&remaining->second.second==order){pending_.erase(remaining->second);targets_.erase(remaining);}}
  }
  clock_=now;return true;
 }
};
}
