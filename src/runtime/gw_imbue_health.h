#pragma once
#include "gw_heal_effects.h"
#include <algorithm>

namespace fw::gw::skills::effects {

struct ImbueHealthPlan {
    unsigned rank=0;
    unsigned percent=0;
    unsigned cap=300;
    auto operator<=>(const ImbueHealthPlan&) const = default;
};

inline std::optional<ImbueHealthPlan> imbue_health(unsigned rank) {
    const auto* row=find_skill(1526);
    if(!row||rank>20||row->type!=5||row->profession!=10||
       row->attribute!=44||row->energy_cost()!=10||
       row->activation!=.25f||row->aftercast!=.75f||row->recharge!=10||
       row->scale0!=5||row->scale15!=50||row->bonus0!=300||
       row->caster_body_resource!=1400)return {};
    return ImbueHealthPlan{rank,heal_scale(row->scale0,row->scale15,rank),300};
}

// Pure completion-time amount. Target admission, actual Health changes,
// profession/rank authority, and animation publication belong to World/Runtime.
inline std::optional<double> imbue_health_amount(const ImbueHealthPlan& plan,
                                                 double caster_current_health) {
    if(imbue_health(plan.rank)!=plan||!std::isfinite(caster_current_health)||
       caster_current_health<=0)return {};
    return std::min(double(plan.cap),
                    caster_current_health*plan.percent/100.);
}

}
