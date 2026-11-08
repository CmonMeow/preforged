#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct FlameDjinnHastePlan {
 unsigned rank=0,damage=15;
 std::uint64_t duration=8000;
 float radius=166,movement=1.25f;
 auto operator<=>(const FlameDjinnHastePlan&)const=default;
};
inline std::optional<FlameDjinnHastePlan> flame_djinn_haste(unsigned rank){
 const auto* r=find_skill(1381);
 if(rank>20||!r||r->campaign!=3||r->profession!=6||r->attribute!=10||r->type!=6||r->special||
    r->energy_cost()!=10||r->health_sacrifice||r->adrenaline||r->activation!=.75f||r->aftercast!=.75f||r->recharge!=20||
    r->scale0!=15||r->scale15!=120||r->bonus0!=25||r->bonus15!=25||r->duration0!=8||r->duration15!=14||
    r->caster_body_resource!=1638||r->target_body_resource!=2077)return {};
 return FlameDjinnHastePlan{rank,interpolate(15,120,rank),1000ull*interpolate(8,14,rank)};
}
}
