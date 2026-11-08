#pragma once
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct DefyPainPlan {
 unsigned skill=318,rank=0,health_bonus=90,armor=20,reduction=1,adrenaline=120;
 std::uint64_t duration=20000;
 bool stance=false;
 auto operator<=>(const DefyPainPlan&)const=default;
};
inline std::optional<DefyPainPlan> defy_pain(unsigned rank,bool pvp=false){
 const unsigned skill=pvp?3204:318;const auto* r=find_skill(skill);
 if(rank>20||!r||r->profession!=1||r->attribute!=17||r->type!=(pvp?3u:16u)||r->special!=(pvp?4194308u:4u)||
    r->energy_cost()!=0||r->health_sacrifice||r->adrenaline!=120||r->activation!=0||r->aftercast!=0||r->recharge!=0||
    r->scale0!=90||r->scale15!=300||r->bonus0!=(pvp?0u:1u)||r->bonus15!=(pvp?0u:10u)||
    r->duration0!=20||r->duration15!=20||r->caster_body_resource!=595||r->target_body_resource!=2077||r->pvp_skill!=(pvp?318u:3204u))return {};
 return DefyPainPlan{skill,rank,interpolate(90,300,rank),20,pvp?0u:interpolate(1,10,rank),120,20000,pvp};
}
}
