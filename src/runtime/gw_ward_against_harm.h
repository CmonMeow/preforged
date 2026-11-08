#pragma once
#include "gw_stationary_ground_area.h"
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <algorithm>
#include <compare>
#include <span>
namespace fw::gw::skills::effects {
struct WardHarmPlan {
 unsigned skill=239,rank=0,energy=15,armor=12,elemental_armor=12,regeneration=1;
 Time duration=5000,activation=1000,aftercast=750,recharge=20000;
 float radius=322;bool pvp=false;
 auto operator<=>(const WardHarmPlan&)const=default;
};
inline std::optional<WardHarmPlan> ward_harm_plan(unsigned skill,unsigned rank){
 const bool pvp=skill==2806;const auto* row=find_skill(skill);
 if((skill!=239&&!pvp)||rank>20||!row||row->campaign!=1||row->profession!=6||row->attribute!=11||
    row->type!=11||row->special!=(pvp?4194308u:4u)||row->energy_cost()!=15||row->health_sacrifice||
    row->activation!=1.f||row->aftercast!=.75f||row->recharge!=(pvp?30u:20u)||
    row->pvp_skill!=(pvp?239u:2806u)||row->target||row->duration0!=5||row->duration15!=15||
    row->scale0!=12||row->scale15!=24||row->bonus0!=1||row->bonus15!=3||
    row->caster_body_resource!=420||row->target_body_resource!=2077)return {};
 WardHarmPlan plan;plan.skill=skill;plan.rank=rank;plan.armor=plan.elemental_armor=interpolate(12,24,rank);
 plan.regeneration=interpolate(1,3,rank);plan.duration=Time(interpolate(5,15,rank))*1000;
 plan.recharge=pvp?30000:20000;plan.pvp=pvp;return plan;
}
struct WardAgainstHarm {StationaryGroundArea area;WardHarmPlan plan;};
inline std::optional<WardAgainstHarm> ward_against_harm(Scope caster,unsigned effect,std::uint64_t team,
        unsigned plane,float x,float y,const WardHarmPlan& plan,Time now){
 const auto canonical=ward_harm_plan(plan.skill,plan.rank);if(!canonical||*canonical!=plan)return {};
 const auto area=stationary_ground_area(caster,effect,team,plane,x,y,plan.radius,plan.duration,now);
 if(!area)return {};return WardAgainstHarm{*area,plan};
}
inline bool valid_ward_harm(const WardAgainstHarm& ward){
 const auto canonical=ward_against_harm(ward.area.caster,ward.area.effect,ward.area.team,ward.area.plane,
     ward.area.x,ward.area.y,ward.plan,ward.area.created);
 return canonical&&canonical->area.expires==ward.area.expires&&canonical->area.radius==ward.area.radius;
}
struct WardHarmBenefits {
 unsigned armor=0,elemental_armor=0,regeneration=0;
 auto operator<=>(const WardHarmBenefits&)const=default;
};
// Same-family overlap grants one strongest benefit, not a pip/armor multiplier.
// This is explicit private overlap policy; no retail overlap capture proves it.
inline WardHarmBenefits ward_harm_benefits(std::span<const WardAgainstHarm> wards,
        const GroundAreaRecipient& recipient,Time now){
 WardHarmBenefits out;
 for(const auto& ward:wards)if(valid_ward_harm(ward)&&ground_area_covers(ward.area,recipient,now)){
  out.armor=std::max(out.armor,ward.plan.armor);out.elemental_armor=std::max(out.elemental_armor,ward.plan.elemental_armor);
  out.regeneration=std::max(out.regeneration,ward.plan.regeneration);
 }
 return out;
}
// Return ONE skill-armor contribution. The general+elemental parts of this
// single ward combine before the existing skill-armor cap/strongest-source
// resolver. Capping two separate24 sources would incorrectly reduce48 to25.
inline std::optional<unsigned> ward_harm_skill_armor(const WardHarmBenefits& benefit,combat::Kind kind,bool ignores_armor=false){
 bool valid=benefit==WardHarmBenefits{};
 for(unsigned rank=0;rank<=20&&!valid;++rank)valid=benefit==WardHarmBenefits{
     interpolate(12,24,rank),interpolate(12,24,rank),interpolate(1,3,rank)};
 if(!valid)return {};
 if(ignores_armor||kind==combat::Kind::armor_ignoring)return 0;
 switch(kind){
 case combat::Kind::earth:case combat::Kind::fire:case combat::Kind::cold:case combat::Kind::lightning:
  return benefit.armor+benefit.elemental_armor;
 case combat::Kind::piercing:case combat::Kind::slashing:case combat::Kind::holy:return benefit.armor;
 default:return {};
 }
}
}
