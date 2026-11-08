#pragma once
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <compare>
#include <optional>

namespace fw::gw::skills::effects {

struct MendBodySoulPlan {
    unsigned rank=0,heal=0;
    unsigned earshot=1012; // Shared private-server convention; retail boundary unmeasured.
    auto operator<=>(const MendBodySoulPlan&)const=default;
};

inline std::optional<MendBodySoulPlan> mend_body_soul(unsigned rank){
    const auto* row=find_skill(1234);
    if(rank>20||!row||row->campaign!=2||row->profession!=8||row->attribute!=33||
       row->type!=5||row->special||row->energy_cost()!=5||row->activation!=.75f||
       row->aftercast!=.75f||row->recharge!=3||row->target!=3||
       row->scale0!=20||row->scale15!=115||row->caster_body_resource!=1090)return {};
    return MendBodySoulPlan{rank,interpolate(20,115,rank)};
}

struct MendBodySoulQuote {unsigned heal=0,conditions_to_remove=0;};

// The World must supply a completion-time count of live, typed spirits near
// the recipient. Unknown creature types cannot be silently counted as zero.
inline std::optional<MendBodySoulQuote> quote_mend_body_soul(
        const MendBodySoulPlan& plan,std::optional<unsigned> nearby_spirits,
        unsigned active_conditions){
    if(mend_body_soul(plan.rank)!=plan||!nearby_spirits||active_conditions>10)return {};
    return MendBodySoulQuote{plan.heal,std::min(*nearby_spirits,active_conditions)};
}

}
