#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct StoneDaggersPlan {
 unsigned rank=0,projectiles=2,model=306;
 double damage=8;
 std::uint64_t bleeding_ms=1000;
 float range=1248;
 auto operator<=>(const StoneDaggersPlan&)const=default;
};
inline std::optional<StoneDaggersPlan> stone_daggers(unsigned rank){
 const auto* row=find_skill(172);
 if(rank>20||!row||row->type!=5||row->attribute!=9||row->encoded_energy!=5||row->activation!=1.f||
    row->aftercast!=.75f||row->recharge||row->pvp_skill!=3433||row->scale0!=8||row->scale15!=33||
    row->duration0!=1||row->duration15!=5||row->bonus0!=1800||row->bonus15!=1800||row->caster_body_resource!=307)return {};
 return StoneDaggersPlan{rank,2,306,damage_scale(8,33,rank),std::uint64_t(interpolate(1,5,rank))*1000,1248};
}
struct StoneDaggerHit {
 bool accepted=false;combat::Actor actor;combat::Hit hit;
 combat::conditions::State conditions;bool bleeding_applied=false;
};
// One actual projectile impact, never twice the damage in one armor/protection
// calculation. Flight/miss/scope and the time at which Overcast is sampled
// belong to the authoritative launcher, not to this immutable damage proposal.
inline StoneDaggerHit stone_dagger_hit(const StoneDaggersPlan& plan,combat::Actor actor,
 const combat::conditions::State& conditions,unsigned level,unsigned source,
 std::optional<bool> overcast,std::optional<bool> allows_conditions,std::uint64_t now,const combat::DamageProjection& projection={}){
 const auto expected=stone_daggers(plan.rank);
 if(!expected||plan!=*expected||!source||source>65535||!actor.id||source==actor.id||!actor.generation||!actor.earth_armor||!overcast)return {};
 actor.earth_armor=conditions.armor(*actor.earth_armor,now);
 const auto step=combat::project_damage(actor,{combat::Kind::earth,plan.damage,level,172},projection);
 if(!step)return {};
 const auto hit=step->incoming;
 StoneDaggerHit result{true,actor,hit,conditions,false};
 if(actor.health->current>0&&*overcast){
  if(!allows_conditions)return {};
  if(*allows_conditions){if(!result.conditions.apply(combat::conditions::Kind::bleeding,source,now,plan.bleeding_ms))return {};
   result.bleeding_applied=true;}
 }
 return result;
}
}
