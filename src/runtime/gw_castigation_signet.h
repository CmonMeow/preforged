#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct CastigationSignetPlan {
 unsigned rank=0,damage=0,energy=0;float range=1248;
 auto operator<=>(const CastigationSignetPlan&)const=default;
};
inline std::optional<CastigationSignetPlan> castigation_signet(unsigned rank){
 const auto* r=find_skill(2006);
 if(rank>20||!r||r->type!=7||r->profession!=3||r->attribute!=14||r->special||r->health_sacrifice||r->adrenaline||
    r->energy_cost()!=0||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=20||
    r->scale0!=26||r->scale15!=56||r->bonus0!=1||r->bonus15!=10||
    r->caster_body_resource!=2077||r->target_body_resource!=1829)return {};
 return CastigationSignetPlan{rank,26+2*rank,interpolate(1,10,rank)};
}
inline std::optional<unsigned> castigation_energy(const CastigationSignetPlan& plan,bool attacking){
 const auto canonical=castigation_signet(plan.rank);if(!canonical||*canonical!=plan)return {};
 return attacking?plan.energy:0u;
}
}
