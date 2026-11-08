#pragma once
#include "gw_holy_damage.h"
#include "gw_effect_registry.h"
#include "gw_projectile_flight.h"
namespace fw::gw::skills::effects {
// Spear of Light1130: the bonus is a separate holy hit, evaluated at impact.
// This plan does not turn a projectile into immediate damage. Delivery/LOS,
// target lifetime and impact-time attacking facts belong to World.
struct SpearLight {
 unsigned rank=0,damage=0,attacking_bonus=0;
 auto operator<=>(const SpearLight&)const=default;
};
inline std::optional<SpearLight> spear_light(unsigned rank){
 const auto* r=find_skill(1130);
 if(rank>20||!r||r->profession!=3||r->attribute!=14||r->type!=5||r->special||
    r->energy_cost()!=5||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=15||
    r->scale0!=26||r->scale15!=56||r->bonus0!=15||r->bonus15!=60||
    r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 return SpearLight{rank,26+2*rank,15+3*rank};
}
inline std::optional<std::vector<combat::Damage>> spear_light_hits(
 const SpearLight& plan,const HolyTraits& traits,unsigned level,std::optional<bool> attacking){
 const auto expected=spear_light(plan.rank);
 if(!expected||*expected!=plan||!traits.light_sensitive||!attacking)return {};
 const unsigned multiplier=*traits.light_sensitive?2u:1u;
 std::vector<combat::Damage> hits{{combat::Kind::armor_ignoring,double(plan.damage*multiplier),level,1130}};
 if(*attacking)hits.push_back({combat::Kind::armor_ignoring,double(plan.attacking_bonus*multiplier),level,1130});
 for(const auto& hit:hits)if(!combat::valid(hit))return {};
 return hits;
}
struct SpearLightActor {
 Scope scope;float x=0,y=0;std::uint16_t plane=0;bool alive=false;
};
struct SpearLightTarget : SpearLightActor {
 bool hostile=false;HolyTraits traits;std::optional<bool> attacking;
};
struct SpearLightImpact {Scope caster,target;std::uint64_t at;std::vector<combat::Damage> hits;};
class SpearLightFlight {
 SpearLight plan_;Scope caster_,target_;projectiles::Policy policy_;
 projectiles::Aim aim_{};std::uint16_t plane_=0;std::uint64_t arrives_=0,last_=0;bool pending_=false;
public:
 bool launch(const SpearLight& plan,const SpearLightActor& source,const SpearLightTarget& target,
             projectiles::Policy flight,double vx,double vy,std::uint64_t now){
  const auto expected=spear_light(plan.rank);
  if(pending_||!expected||*expected!=plan||!source.scope.instance||!source.scope.actor||!source.scope.generation||
     target.scope.instance!=source.scope.instance||!target.scope.actor||!target.scope.generation||
     source.scope.actor==target.scope.actor||!source.alive||!target.alive||!target.hostile||source.plane!=target.plane||
     !std::isfinite(source.x)||!std::isfinite(source.y)||!std::isfinite(target.x)||!std::isfinite(target.y)||
     std::hypot(target.x-source.x,target.y-source.y)>1248.f)return false;
  const auto aim=projectiles::intercept(flight,source.x,source.y,target.x,target.y,vx,vy);
  if(!aim||now>UINT64_MAX-aim->flight_ms)return false;
  plan_=plan;caster_=source.scope;target_=target.scope;plane_=source.plane;policy_=flight;
  aim_=*aim;arrives_=now+aim->flight_ms;last_=now;pending_=true;return true;
 }
 // Rebase arrival to actual visual publication without changing aim/duration.
 bool publish_at(std::uint64_t now){
  if(!pending_||now<last_||now-last_>30500||now>UINT64_MAX-aim_.flight_ms)return false;
  last_=now;arrives_=now+aim_.flight_ms;return true;
 }
 bool pending()const{return pending_;}
 auto arrives()const{return arrives_;}
 const auto& aim()const{return aim_;}
 void cancel(){pending_=false;}
 std::optional<SpearLightImpact> impact(const SpearLightActor& source,const SpearLightTarget& target,unsigned level,std::uint64_t now){
  if(!pending_)return {};
  if(now<last_||source.scope!=caster_||target.scope!=target_){cancel();return {};}
  last_=now;if(now<arrives_)return {};
  cancel();
  if(!source.alive||!target.alive||!target.hostile||source.plane!=plane_||target.plane!=plane_||
     !std::isfinite(target.x)||!std::isfinite(target.y)||std::hypot(target.x-aim_.x,target.y-aim_.y)>policy_.hit_radius)return {};
  auto hits=spear_light_hits(plan_,target.traits,level,target.attacking);
  if(!hits)return {};
  return SpearLightImpact{caster_,target_,arrives_,std::move(*hits)};
 }
};

}
