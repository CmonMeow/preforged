#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <algorithm>
#include <cmath>
#include <compare>
#include <optional>
#include <set>
#include <vector>

namespace fw::gw::skills::effects {

struct HealingRibbonPlan {
    unsigned rank=0,primary_heal=0,secondary_heal=0;
    unsigned range=1248,near=252; // Near redirects to Nearby,not Area;retail boundary unmeasured.
    auto operator<=>(const HealingRibbonPlan&)const=default;
};

inline std::optional<HealingRibbonPlan> healing_ribbon(unsigned rank){
    const auto* row=find_skill(2062);
    if(rank>20||!row||row->campaign!=4||row->profession!=3||
       row->attribute!=13||row->type!=5||row->special||
       row->energy_cost()!=10||row->activation!=1.f||
       row->aftercast!=.75f||row->recharge!=5||row->target!=4||
       row->scale0!=20||row->scale15!=110||
       row->bonus0!=10||row->bonus15!=100||
       row->caster_body_resource!=2077||row->target_body_resource!=1738)return {};
    return HealingRibbonPlan{rank,interpolate(20,110,rank),
                             interpolate(10,100,rank)};
}

struct HealingRibbonCandidate {
    Scope scope;
    float distance_from_primary=0;
    bool alive=false,allied=false;
};

struct HealingRibbonRecipient {
    Scope scope;
    unsigned base_heal=0;
    bool divine_favor=false;
    auto operator<=>(const HealingRibbonRecipient&)const=default;
};

// The spell targets another ally. The target receives the larger heal and
// Divine Favor; up to two distinct nearby allies receive only the smaller
// heal. Nearest-first selection is a private-server policy pending a retail
// recipient-order capture. The caster may be a secondary recipient.
inline std::optional<std::vector<HealingRibbonRecipient>> healing_ribbon_recipients(
        const HealingRibbonPlan& plan,Scope caster,Scope primary,
        std::vector<HealingRibbonCandidate> candidates){
    if(healing_ribbon(plan.rank)!=plan||!caster.instance||!caster.actor||
       !caster.generation||!primary.instance||!primary.actor||
       !primary.generation||caster.instance!=primary.instance||
       caster==primary)return {};
    const auto selected=std::find_if(candidates.begin(),candidates.end(),
        [&](const auto& c){return c.scope==primary;});
    if(selected==candidates.end()||!selected->alive||!selected->allied)return {};
    std::set<Scope> seen{primary};
    std::vector<HealingRibbonCandidate> extras;
    for(const auto& c:candidates){
        if(c.scope==primary||c.scope.instance!=primary.instance||
           !c.scope.actor||!c.scope.generation||!c.alive||!c.allied||
           !std::isfinite(c.distance_from_primary)||
           c.distance_from_primary<0||c.distance_from_primary>plan.near||
           !seen.insert(c.scope).second)continue;
        extras.push_back(c);
    }
    std::sort(extras.begin(),extras.end(),[](const auto& a,const auto& b){
        if(a.distance_from_primary!=b.distance_from_primary)
            return a.distance_from_primary<b.distance_from_primary;
        return a.scope<b.scope;
    });
    std::vector<HealingRibbonRecipient> result{{primary,plan.primary_heal,true}};
    for(const auto& c:extras){
        if(result.size()==3)break;
        result.push_back({c.scope,plan.secondary_heal,false});
    }
    return result;
}

}
