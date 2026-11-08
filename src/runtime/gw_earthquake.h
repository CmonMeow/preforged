#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>

namespace fw::gw::skills::effects {
struct EarthquakePlan {
 unsigned skill=0,rank=0;double damage=0;float range=1248,radius=252;
 unsigned knockdown_ms=2000;
 auto operator<=>(const EarthquakePlan&)const=default;
};
inline std::optional<EarthquakePlan> earthquake(unsigned skill,unsigned rank){
 const auto* row=find_skill(skill);
 if((skill!=170&&skill!=1086)||rank>20||!row||row->type!=5||row->profession!=6||row->attribute!=9||
    row->energy_cost()!=25||!(row->special&1)||row->overcast!=10||row->activation!=3.f||
    row->aftercast!=.75f||row->recharge!=15||row->scale0!=26||row->scale15!=100||
    row->caster_body_resource!=303||row->target_body_resource!=2077)return {};
 return EarthquakePlan{skill,rank,damage_scale(26,100,rank)};
}
struct EarthquakeResult {combat::Actor actor;combat::Hit hit;bool knockdown=false;};
// Proposal only: World must preflight all occupants and control reservations
// before committing any damage. Immunity is distinct from unknown authority.
inline std::optional<EarthquakeResult> earthquake_target(const EarthquakePlan& plan,combat::Actor actor,
 const combat::conditions::State& conditions,unsigned level,std::uint64_t now,std::optional<bool> allows_knockdown,const combat::DamageProjection& projection={}){
 const auto expected=earthquake(plan.skill,plan.rank);
 if(!expected||*expected!=plan||!actor.earth_armor)return {};
 actor.earth_armor=conditions.armor(*actor.earth_armor,now);
 const auto step=combat::project_damage(actor,{combat::Kind::earth,plan.damage,level,plan.skill},projection);
 if(!step||(actor.health->current>0&&!allows_knockdown))return {};
 return EarthquakeResult{actor,step->incoming,actor.health->current>0&&*allows_knockdown};
}
}
