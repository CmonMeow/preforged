#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include "gw_combat.h"
#include "gw_effect_registry.h"
#include <map>
namespace fw::gw::skills::effects {
struct SpiritRift {
 unsigned rank=0;double damage=25;Time delay=3000,cracked_ms=1000;float radius=166;
 auto operator<=>(const SpiritRift&)const=default;
};
inline std::optional<SpiritRift> spirit_rift(unsigned rank){
 const auto* r=find_skill(910);
 if(rank>20||!r||r->profession!=8||r->attribute!=34||r->type!=5||r->special||r->energy_cost()!=10||
    r->activation!=2.f||r->aftercast!=.75f||r->recharge!=5||r->scale0!=25||r->scale15!=125||
    r->bonus0!=1||r->bonus15!=20||r->duration0!=3||r->duration15!=3||
    r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 return SpiritRift{rank,damage_scale(25,125,rank),3000,Time(interpolate(1,20,rank))*1000,166};
}
struct SpiritRiftHit {
 combat::Actor actor;combat::conditions::State conditions;combat::Hit hit;bool cracked_added=false;
};
// Apply existing Cracked Armor to this hit. Newly inflicted Cracked Armor is
// installed only after damage on a surviving recipient; it cannot amplify
// its own first hit. Armor penetration is zero, not Air Magic's25percent.
inline std::optional<SpiritRiftHit> spirit_rift_hit(const SpiritRift& plan,combat::Actor actor,
 combat::conditions::State conditions,unsigned level,unsigned caster,Time now,bool allows_conditions,const combat::DamageProjection& projection={}){
 const auto expected=spirit_rift(plan.rank);
 if(!expected||*expected!=plan||!caster||!actor.lightning_armor||now>UINT64_MAX-plan.cracked_ms)return {};
 actor.lightning_armor=conditions.armor(*actor.lightning_armor,now);
 auto private_projection=projection;const auto step=combat::project_damage(actor,{combat::Kind::lightning,plan.damage,level,910,0},private_projection);
 if(!step)return {};const auto& hit=step->incoming;
 SpiritRiftHit result{actor,std::move(conditions),hit,false};
 if(actor.health->current>0&&allows_conditions){
  if(!result.conditions.apply(combat::conditions::Kind::cracked_armor,caster,now,plan.cracked_ms))return {};
  result.cracked_added=true;
 }
 return result;
}
struct SpiritRiftPulse {Scope caster;unsigned level;float x,y;std::uint16_t plane;SpiritRift plan;Time at;};
// One-shot immutable ground centers. Recipient membership is sampled at due
// time by World, never stored as the originally selected actor.
class SpiritRiftSchedule {
 std::map<std::pair<Time,std::uint64_t>,SpiritRiftPulse> pending_;
 std::uint64_t next_=1;Time clock_=0;
public:
 bool add(Scope caster,unsigned level,float x,float y,std::uint16_t plane,const SpiritRift& plan,Time now){
  const auto expected=spirit_rift(plan.rank);
  if(!expected||*expected!=plan||!caster.instance||!caster.actor||!caster.generation||!level||level>255||
     !std::isfinite(x)||!std::isfinite(y)||now<clock_||now>UINT64_MAX-plan.delay||next_==UINT64_MAX||pending_.size()>=4096)return false;
  pending_.emplace(std::pair{now+plan.delay,next_++},SpiritRiftPulse{caster,level,x,y,plane,plan,now+plan.delay});return true;
 }
 std::optional<Time> next_deadline()const{return pending_.empty()?std::nullopt:std::optional(pending_.begin()->first.first);}
 std::size_t size()const{return pending_.size();}
 void retire(Scope caster){std::erase_if(pending_,[&](const auto& p){return p.second.caster==caster;});}
 template<class Apply>void advance(Time now,Apply&& apply){
  if(now<clock_)return;clock_=now;
  while(!pending_.empty()&&pending_.begin()->first.first<=now){auto pulse=pending_.begin()->second;pending_.erase(pending_.begin());apply(pulse);}
 }
};
}
