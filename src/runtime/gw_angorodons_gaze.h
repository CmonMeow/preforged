#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct AngorodonsGazePlan {
 unsigned rank=0,amount=0,energy=0;float range=1248;
 auto operator<=>(const AngorodonsGazePlan&)const=default;
};
inline std::optional<AngorodonsGazePlan> angorodons_gaze(unsigned rank){
 const auto* r=find_skill(2189);
 if(rank>20||!r||r->type!=5||r->profession!=4||r->attribute!=6||r->special||r->health_sacrifice||
    r->energy_cost()!=5||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=10||
    r->scale0!=10||r->scale15!=50||r->bonus0!=3||r->bonus15!=12||
    r->caster_body_resource!=2077||r->target_body_resource!=1818)return {};
 return AngorodonsGazePlan{rank,interpolate(10,50,rank),interpolate(3,12,rank)};
}
// Read the caster's actual conditions immediately before the transfer. This
// is a proposal only;World owns clocks,scope,transfer and precise Energy gain.
inline std::optional<unsigned> angorodons_gaze_energy(const AngorodonsGazePlan& plan,
 const combat::conditions::State& caster,std::uint64_t now){
 const auto canonical=angorodons_gaze(plan.rank);if(!canonical||*canonical!=plan)return {};
 return caster.count(now)?plan.energy:0u;
}
}
