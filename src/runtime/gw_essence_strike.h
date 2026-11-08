#pragma once

#include "gw_combat.h"
#include "gw_skill_effect_plan.h"
#include "gw_spirit_light.h"
#include <compare>
#include <optional>

namespace fw::gw::skills::effects {
struct EssenceStrikePlan {
    unsigned rank=0;
    double lightning_damage=15;
    unsigned spirit_energy=1;
    unsigned earshot=1012;
    float range=1248.f;
    auto operator<=>(const EssenceStrikePlan&)const=default;
};

inline std::optional<EssenceStrikePlan> essence_strike(unsigned rank){
    const auto* row=find_skill(1227);
    if(!row||rank>20||row->campaign!=2||row->profession!=8||
       row->attribute!=34||row->type!=5||row->energy_cost()!=5||
       row->activation!=1.f||row->aftercast!=.75f||row->recharge!=8||
       row->scale0!=15||row->scale15!=60||row->bonus0!=1||row->bonus15!=9||
       row->caster_body_resource!=2077||row->target_body_resource!=1086)return {};
    return EssenceStrikePlan{rank,damage_scale(15,60,rank),
        interpolate(1,9,rank)};
}

struct EssenceStrikeResult {
    combat::Actor target;
    combat::Hit hit;
    unsigned energy_gain=0;
};

// World supplies a current spirit census and atomic damage/Energy publication.
// Unknown proximity must not be converted to absence or a free Energy grant.
inline std::optional<EssenceStrikeResult> essence_strike_target(
        const EssenceStrikePlan& plan,combat::Actor target,unsigned level,
        SpiritProximity proximity){
    if(essence_strike(plan.rank)!=plan||proximity==SpiritProximity::unknown)return {};
    const auto hit=combat::apply(target,{combat::Kind::lightning,
        plan.lightning_damage,level,1227},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    return EssenceStrikeResult{target,hit,
        proximity==SpiritProximity::present?plan.spirit_energy:0u};
}
}
