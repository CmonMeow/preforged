#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct MarkOfDeathPlan {
 unsigned rank=0,reduction=33;std::uint64_t duration=0;float range=1248;
 auto operator<=>(const MarkOfDeathPlan&)const=default;
};
inline std::optional<MarkOfDeathPlan> mark_of_death(unsigned rank){
 const auto* r=find_skill(785);
 if(rank>20||!r||r->type!=4||r->profession!=7||r->attribute!=30||r->special||r->health_sacrifice||
    r->energy_cost()!=10||r->activation!=.25f||r->aftercast!=.75f||r->recharge!=20||
    r->scale0!=33||r->scale15!=33||r->duration0!=4||r->duration15!=10||
    r->caster_body_resource!=2077||r->target_body_resource!=837)return {};
 return MarkOfDeathPlan{rank,33,std::uint64_t(interpolate(4,10,rank))*1000,1248};
}
}
