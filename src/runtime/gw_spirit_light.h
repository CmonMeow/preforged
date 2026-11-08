#pragma once

#include "gw_health_sacrifice.h"
#include <cmath>
#include <optional>

namespace fw::gw::skills::effects {

struct SpiritLightPlan {
    unsigned rank=0;
    unsigned heal=0;
    unsigned energy=5;
    unsigned sacrifice_percent=17;
    unsigned caster_body=2077;
    unsigned target_body=1089;
    unsigned activation_ms=1000;
    unsigned aftercast_ms=750;
    unsigned recharge_ms=4000;
    unsigned earshot=1012; // Existing private earshot convention; not measured for this cast.
    bool operator==(const SpiritLightPlan&)const=default;
};

inline std::optional<SpiritLightPlan> spirit_light(unsigned rank){
    const auto* row=find_skill(915);
    if(!row||rank>20||row->campaign!=2||row->profession!=8||row->attribute!=33||
       row->type!=5||row->special||row->energy_cost()!=5||row->health_sacrifice!=17||
       row->adrenaline||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=4||
       row->target!=3||row->scale0!=60||row->scale15!=180||row->caster_body_resource!=2077||
       row->target_body_resource!=1089)return {};
    SpiritLightPlan plan;
    plan.rank=rank;
    plan.heal=static_cast<unsigned>(std::lround(60.+120.*rank/15.));
    return plan;
}

enum class SpiritProximity {unknown,absent,present};

struct SpiritLightQuote {
    unsigned raw_heal=0;
    std::optional<sacrifice::Loss> sacrifice;
};

// The caller must supply an authoritative, completion-time spirit query.
// Unknown proximity is not equivalent to an empty area. This is a quote,
// not payment, target admission, healing or packet publication.
inline std::optional<SpiritLightQuote> quote_spirit_light(
        const SpiritLightPlan& plan,SpiritProximity proximity,
        double caster_health,unsigned caster_max_health){
    if(spirit_light(plan.rank)!=plan||proximity==SpiritProximity::unknown||
       !std::isfinite(caster_health)||!caster_max_health||caster_health<=0||
       caster_health>caster_max_health)return {};
    SpiritLightQuote quote{plan.heal,{}};
    if(proximity==SpiritProximity::absent){
        quote.sacrifice=sacrifice::resolve(915,caster_health,caster_max_health);
        if(!quote.sacrifice)return {};
    }
    return quote;
}

}
