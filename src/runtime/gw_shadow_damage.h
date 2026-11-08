#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_life_stealing.h"
#include <compare>
namespace fw::gw::skills::effects {
struct ShadowPlan{unsigned skill=0,rank=0,damage=0,steal_above_half=0;float range=1248;auto operator<=>(const ShadowPlan&)const=default;};
inline std::optional<ShadowPlan> shadow_damage(unsigned skill,unsigned rank=0){
 if(rank>20)return {};if(skill==133)return ShadowPlan{skill,rank,interpolate(10,48,rank),0};
 // Obsidian Flame's earth attribute does not make its damage armor-respecting.
 if(skill==219)return ShadowPlan{skill,rank,interpolate(22,112,rank),0};
 // Vile Touch is untyped, armor-ignoring damage; it does not steal life.
 if(skill==155)return ShadowPlan{skill,rank,interpolate(20,65,rank),0,144};
 if(skill==158||skill==1078)return ShadowPlan{skill,rank,interpolate(20,58,rank),0,144};
 if(skill==102||skill==1067)return ShadowPlan{skill,rank,interpolate(12,48,rank),interpolate(12,48,rank)};return {};
}
struct ShadowResult{combat::Health source;combat::Actor target;combat::Hit hit;std::optional<LifeStealResult> steal;};
inline std::optional<ShadowResult> shadow_target(const ShadowPlan& p,combat::Health source,combat::Actor target,unsigned level,const combat::DamageProjection& projection={}){
 const auto expected=shadow_damage(p.skill,p.rank);if(!expected||*expected!=p||!combat::valid(source)||source.current<=0||!target.health)return {};
 const bool above=target.health->current>double(target.health->maximum)/2;
 auto damage_projection=projection;const auto step=combat::project_damage(target,{combat::Kind::armor_ignoring,double(p.damage),level,p.skill},damage_projection);if(!step)return {};const auto& hit=step->incoming;
 ShadowResult result{source,target,hit,{}};
 if(above&&p.steal_above_half&&target.health->current>0){LifeStealResult steal{source,*target.health};steal.stolen=std::min(std::max(0.,double(p.steal_above_half)-target.life_steal_reduction.value_or(target.incoming_reduction)),target.health->current+1);steal.gained=std::min(steal.stolen,source.maximum-source.current);steal.source.current+=steal.gained;steal.target.current=std::max(0.,steal.target.current-steal.stolen);steal.killed=!steal.target.current;result.steal=steal;result.source=steal.source;result.target.health=steal.target;}
 return result;
}
}
