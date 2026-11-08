#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_natural_hex_timers.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct LightningSurgePlan {
 unsigned rank=0;double damage=14;unsigned penetration=25;
 Time duration=3000,condition_ms=5000,knockdown_ms=2000;float range=1248;
 auto operator<=>(const LightningSurgePlan&)const=default;
};
inline std::optional<LightningSurgePlan> lightning_surge(unsigned rank){
 const auto* row=find_skill(205);
 if(rank>20||!row||row->type!=4||row->attribute!=8||row->scale0!=14||row->scale15!=100||
    row->bonus0!=5||row->bonus15!=20||row->duration0!=3||row->duration15!=3||
    row->encoded_energy!=10||row->activation!=1||row->recharge!=10)return {};
 return LightningSurgePlan{rank,damage_scale(14,100,rank),25,3000,Time(interpolate(5,20,rank))*1000};
}
using LightningSurgeTimers=NaturalHexTimers<lightning_surge>;
struct LightningSurgeImpact {
 bool accepted=false,knockdown_eligible=false;combat::Actor actor;
 combat::conditions::State conditions;combat::Hit hit;
};
// Expiry preflight for the World-owned timer. Admission must install the
// owned hex/timer without an initial hit. Resolve on the original deadline,
// using current target armor; early hex removal cancels the whole end effect.
// World must independently handle control immunity, reactions and publication.
inline LightningSurgeImpact lightning_surge_impact(const LightningSurgePlan& plan,combat::Actor actor,
 combat::conditions::State conditions,unsigned level,unsigned caster,Time deadline,bool allows_conditions=true,const combat::DamageProjection& projection={}){
 const auto canonical=lightning_surge(plan.rank);
 if(!canonical||*canonical!=plan||!caster||!actor.lightning_armor)return {};
 actor.lightning_armor=conditions.armor(*actor.lightning_armor,deadline);
 const auto step=combat::project_damage(actor,{combat::Kind::lightning,plan.damage,level,205,plan.penetration},projection);
 if(!step)return {};
 LightningSurgeImpact result{true,actor.health->current>0,actor,conditions,step->incoming};
 if(actor.health->current>0&&allows_conditions&&!result.conditions.apply(combat::conditions::Kind::cracked_armor,caster,deadline,plan.condition_ms))return {};
 return result;
}
}
