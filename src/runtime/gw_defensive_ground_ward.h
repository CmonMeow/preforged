#pragma once
#include "gw_stationary_ground_area.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
#include <compare>
#include <optional>
namespace fw::gw::skills::effects {
enum class DefensiveWardEffect {knockdown_protection};
struct DefensiveWardPlan {
 unsigned skill=938,rank=0,energy=10;
 Time duration=10000,activation=1000,aftercast=750,recharge=30000;
 float radius=322;
 DefensiveWardEffect effect=DefensiveWardEffect::knockdown_protection;
 auto operator<=>(const DefensiveWardPlan&)const=default;
};
inline std::optional<DefensiveWardPlan> defensive_ground_ward_plan(unsigned skill,unsigned rank){
 const auto* r=find_skill(skill);
 if(skill!=938||rank>20||!r||r->type!=11||r->campaign!=2||r->profession!=6||r->attribute!=9||
    r->special||r->energy_cost()!=10||r->health_sacrifice||r->activation!=1.f||r->aftercast!=.75f||
    r->recharge!=30||r->target||r->duration0!=10||r->duration15!=25||
    r->caster_body_resource!=1187||r->target_body_resource!=2077)return {};
 return DefensiveWardPlan{skill,rank,10,Time(interpolate(10,25,rank))*1000};
}
struct DefensiveGroundWard {
 Scope caster;unsigned effect=0;std::uint64_t team=0;unsigned plane=0;
 float x=0,y=0;DefensiveWardPlan plan;Time created=0,expires=0;
};
using DefensiveWardRecipient=GroundAreaRecipient;
inline bool defensive_ward_scope(Scope scope){return ground_area_scope(scope);}
inline std::optional<DefensiveGroundWard> defensive_ground_ward(Scope caster,unsigned effect,
        std::uint64_t team,unsigned plane,float x,float y,const DefensiveWardPlan& plan,Time now){
 const auto canonical=defensive_ground_ward_plan(plan.skill,plan.rank);
 if(!canonical||*canonical!=plan||!defensive_ward_scope(caster)||!effect||!team||plane>65535||
    !std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1000000||std::abs(y)>1000000||now>UINT64_MAX-plan.duration)return {};
 return DefensiveGroundWard{caster,effect,team,plane,x,y,plan,now,now+plan.duration};
}
inline bool defensive_ground_ward_covers(const DefensiveGroundWard& ward,
        const DefensiveWardRecipient& recipient,Time now){
 const auto canonical=defensive_ground_ward(ward.caster,ward.effect,ward.team,ward.plane,ward.x,ward.y,ward.plan,ward.created);
 if(!canonical||canonical->expires!=ward.expires)return false;
 const auto area=stationary_ground_area(ward.caster,ward.effect,ward.team,ward.plane,ward.x,ward.y,
     ward.plan.radius,ward.plan.duration,ward.created);
 return area&&ground_area_covers(*area,recipient,now);
}
inline bool defensive_ward_prevents_knockdown(const DefensiveGroundWard& ward,
        const DefensiveWardRecipient& recipient,Time now){
 return ward.plan.effect==DefensiveWardEffect::knockdown_protection&&defensive_ground_ward_covers(ward,recipient,now);
}
}
