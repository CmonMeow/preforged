#pragma once
#include "gw_combat.h"
#include "gw_conditions.h"
#include "gw_skill_effect_plan.h"
#include <compare>
#include <optional>

namespace fw::gw::skills::effects {
struct IronPalmPlan {
    unsigned rank=0;
    double damage=5;
    unsigned knockdown_ms=2000;
    float range=144;
    auto operator<=>(const IronPalmPlan&)const=default;
};

inline std::optional<IronPalmPlan> iron_palm(unsigned rank){
    const auto* row=find_skill(786);
    if(rank>20||!row||row->campaign!=2||row->profession!=7||
       row->attribute!=30||row->type!=10||row->energy_cost()!=5||
       row->activation!=.75f||row->aftercast!=.75f||row->recharge!=20||
       row->target!=5||row->scale0!=5||row->scale15!=50||
       row->caster_body_resource!=857)return {};
    return IronPalmPlan{rank,damage_scale(5,50,rank)};
}

struct IronPalmResult {
    combat::Actor target;
    combat::Hit hit;
    bool knockdown_requested=false;
};

// World supplies a scoped, living hostile and the target's live status at
// completion. An unknown hex state is not equivalent to an unhexed target.
// Control immunity and the lead-chain stage are owned by World/casting.
inline std::optional<IronPalmResult> iron_palm_target(
        const IronPalmPlan& plan,combat::Actor target,
        const combat::conditions::State& conditions,
        std::optional<bool> hexed,unsigned level,std::uint64_t now){
    if(iron_palm(plan.rank)!=plan||!hexed)return {};
    const auto hit=combat::apply(target,
        {combat::Kind::armor_ignoring,plan.damage,level,786},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    return IronPalmResult{target,hit,!hit.killed&&(*hexed||conditions.count(now)>0)};
}
}
