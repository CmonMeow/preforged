#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
#include <optional>

namespace fw::gw::skills::effects {

struct RejuvenationPlan {
    unsigned rank=0,base_heal=15,conditional_heal=15;
    unsigned activation_ms=1000,aftercast_ms=750,recharge_ms=8000;
    float range=1248.f;
    auto operator<=>(const RejuvenationPlan&)const=default;
};

inline std::optional<RejuvenationPlan> signet_rejuvenation(unsigned rank){
    const auto* row=find_skill(887);
    if(rank>20||!row||row->campaign!=2||row->profession!=3||
       row->attribute!=13||row->type!=7||row->special||
       row->energy_cost()!=0||row->health_sacrifice||row->adrenaline||
       row->activation!=1.f||row->aftercast!=.75f||row->recharge!=8||
       row->target!=3||row->scale0!=15||row->scale15!=75||
       row->bonus0!=15||row->bonus15!=75||
       row->caster_body_resource!=2077||row->target_body_resource!=1012)return {};
    const auto heal=interpolate(15,75,rank);
    return RejuvenationPlan{rank,heal,heal};
}

// Query the recipient at completion, not at cast start. An unknown action
// state cannot be silently treated as idle. This is raw signet healing;
// target modifiers and Monk Divine Favor are separate World effects.
inline std::optional<unsigned> quote_signet_rejuvenation(
        const RejuvenationPlan& plan,std::optional<bool> attacking_or_casting_spell){
    if(signet_rejuvenation(plan.rank)!=plan||!attacking_or_casting_spell)return {};
    return plan.base_heal+(*attacking_or_casting_spell?plan.conditional_heal:0);
}
}
