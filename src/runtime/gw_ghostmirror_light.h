#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_spirit_light.h"
#include <compare>
#include <optional>

namespace fw::gw::skills::effects {

struct GhostmirrorLightPlan {
    unsigned rank=0,heal=0;
    unsigned earshot=1012; // Private-server convention; retail boundary unmeasured.
    auto operator<=>(const GhostmirrorLightPlan&)const=default;
};

inline std::optional<GhostmirrorLightPlan> ghostmirror_light(unsigned rank){
    const auto* row=find_skill(1741);
    if(rank>20||!row||row->campaign!=3||row->profession!=8||row->attribute!=33||
       row->type!=5||row->special||row->energy_cost()!=5||row->activation!=1.f||
       row->aftercast!=.75f||row->recharge!=3||row->target!=4||
       row->scale0!=15||row->scale15!=90||row->caster_body_resource!=1574||
       row->target_body_resource!=1575)return {};
    return GhostmirrorLightPlan{rank,interpolate(15,90,rank)};
}

struct GhostmirrorLightQuote {unsigned recipient_heal=0,caster_heal=0;};

inline std::optional<GhostmirrorLightQuote> quote_ghostmirror_light(
        const GhostmirrorLightPlan& plan,SpiritProximity proximity){
    if(ghostmirror_light(plan.rank)!=plan||proximity==SpiritProximity::unknown)return {};
    return GhostmirrorLightQuote{plan.heal,
        proximity==SpiritProximity::present?plan.heal:0u};
}

}
