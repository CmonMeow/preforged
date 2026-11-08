#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct ShockArrowPlan {
 unsigned rank=0,storage=0,penetration=25,energy_if_cracked=5;
 double damage=0;float range=1248;
 auto operator<=>(const ShockArrowPlan&)const=default;
};
inline std::optional<ShockArrowPlan> shock_arrow(unsigned rank,unsigned storage){
 const auto* r=find_skill(1082);
 if(rank>20||storage>20||!r||r->type!=5||r->profession!=6||r->attribute!=8||r->special||r->health_sacrifice||
    r->energy_cost()!=5||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=8||
    r->scale0!=5||r->scale15!=50||r->bonus0!=5||r->bonus15!=5||
    r->caster_body_resource!=1180||r->target_body_resource!=2077)return {};
 return ShockArrowPlan{rank,storage,25,5+storage/2,damage_scale(5,50,rank)};
}
struct ShockArrowResult {combat::Actor actor;combat::Hit hit;unsigned energy=0;};
// One actual projectile impact only. Flight,movement,obstruction,lifetimes and
// caster Energy settlement belong to World. Conditions are sampled before
// damage,so a lethal hit still qualifies for the documented Energy return.
inline std::optional<ShockArrowResult> shock_arrow_target(const ShockArrowPlan& plan,combat::Actor actor,
 const combat::conditions::State& conditions,unsigned level,std::uint64_t now){
 const auto canonical=shock_arrow(plan.rank,plan.storage);
 if(!canonical||*canonical!=plan||!actor.lightning_armor)return {};
 const bool cracked=conditions.has(combat::conditions::Kind::cracked_armor,now);
 actor.lightning_armor=conditions.armor(*actor.lightning_armor,now);
 const auto hit=combat::apply(actor,{combat::Kind::lightning,plan.damage,level,1082,plan.penetration},true);
 if(hit.reason!=combat::Reason::accepted)return {};
 return ShockArrowResult{actor,hit,cracked?plan.energy_if_cracked:0};
}
}
