#pragma once
#include "gw_skill_effect_plan.h"
#include <array>
#include <compare>
#include <cstdint>
#include <limits>
#include <optional>

namespace fw::gw::skills::effects {

struct RayOfJudgmentPlan {
    unsigned rank=0,damage=0,burning_seconds=0;
    unsigned pulses=5;
    std::uint64_t interval_ms=1000,duration_ms=5000;
    float adjacent=166.f,range=1248.f;
    auto operator<=>(const RayOfJudgmentPlan&)const=default;
};

// The row and archived page agree on five pulses, each applying holy damage
// and refreshing Burning. This is a plan, not a cast or area transaction.
inline std::optional<RayOfJudgmentPlan> ray_of_judgment(unsigned rank){
    const auto* row=find_skill(830);
    if(rank>20||!row||row->campaign!=2||row->profession!=3||
       row->attribute!=14||row->type!=5||row->special!=4||
       row->energy_cost()!=10||row->health_sacrifice||row->adrenaline||
       row->activation!=2.f||row->aftercast!=.75f||row->recharge!=20||
       row->target!=16||row->scale0!=5||row->scale15!=45||
       row->bonus0!=1||row->bonus15!=3||row->duration0!=5||
       row->duration15!=5||row->caster_body_resource!=1017||
       row->target_body_resource!=1018)return {};
    return RayOfJudgmentPlan{rank,interpolate(5,45,rank),
                             interpolate(1,3,rank)};
}

inline std::optional<std::array<std::uint64_t,5>> ray_of_judgment_pulses(
        const RayOfJudgmentPlan& plan,std::uint64_t finish){
    if(ray_of_judgment(plan.rank)!=plan||
       finish>std::numeric_limits<std::uint64_t>::max()-plan.duration_ms)return {};
    return std::array<std::uint64_t,5>{finish+1000,finish+2000,
                                      finish+3000,finish+4000,finish+5000};
}
}
