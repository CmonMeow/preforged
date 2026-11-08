#pragma once
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <compare>
namespace fw::gw::skills::effects {
// Pinned2022 row + archived Snow_Storm title progression. This plan alone
// does not enable casting; World must own the fixed target centre/cohort.
struct SnowStormPlan {
 unsigned deldrimor_rank=0,damage=30,pulses=5,energy=10;
 std::uint64_t activation=1000,aftercast=750,recharge=10000,interval=1000;
 float radius=166,range=1248;
 auto operator<=>(const SnowStormPlan&)const=default;
};
inline std::optional<SnowStormPlan> snow_storm(unsigned rank){
 const auto* row=find_skill(2222);
 if(rank>10||!row||row->campaign!=4||row->profession||row->attribute!=51||row->type!=5||
    row->special!=524288||row->energy_cost()!=10||row->health_sacrifice||row->activation!=1.f||
    row->aftercast!=.75f||row->recharge!=10||row->target!=16||row->scale0!=30||row->scale15!=40||
    row->duration0!=5||row->duration15!=5||row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
 // Archived progression: ranks0..5 give30,32,34,36,38,40; ranks6..10 stay40.
 return SnowStormPlan{rank,30+2*std::min(rank,5u)};
}
}
