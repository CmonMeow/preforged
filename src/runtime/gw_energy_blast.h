#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <algorithm>
#include <compare>

namespace fw::gw::skills::effects {
struct EnergyBlastPlan {
    unsigned rank=0,damage_per_energy=1,maximum_damage=130;
    float range=1248.f;
    auto operator<=>(const EnergyBlastPlan&)const=default;
};
inline std::optional<EnergyBlastPlan> energy_blast(unsigned rank){
    const auto* row=find_skill(2193);
    if(rank>20||!row||row->profession!=6||row->attribute!=12||
       row->type!=5||row->energy_cost()!=10||row->activation!=1.5f||
       row->aftercast!=.75f||row->recharge!=20||row->scale0!=1||
       row->scale15!=2||row->bonus0!=130||row->bonus15!=130||
       row->caster_body_resource!=2077||row->target_body_resource!=1820)return {};
    // The pinned row stores integer endpoints. Mid-rank interpolation is a
    // server policy until a rank-controlled retail capture establishes it.
    return EnergyBlastPlan{rank,rank>=8?2u:1u};
}
struct EnergyBlastResult {
    bool accepted=false;
    combat::Actor target;
    std::optional<combat::Hit> hit;
};
inline EnergyBlastResult energy_blast_target(const EnergyBlastPlan& plan,
        combat::Actor target,unsigned caster_energy,unsigned caster_level){
    if(energy_blast(plan.rank)!=plan||!target.health||!combat::valid(*target.health)||
       target.health->current<=0||!target.damageable||!caster_level||caster_level>255||
       caster_energy>1000000)return {};
    const unsigned damage=std::min(plan.maximum_damage,
        caster_energy>plan.maximum_damage/plan.damage_per_energy?
        plan.maximum_damage:caster_energy*plan.damage_per_energy);
    if(!damage)return {true,target,{}};
    const auto hit=combat::apply(target,{combat::Kind::armor_ignoring,
        double(damage),caster_level,2193},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    return {true,target,hit};
}
}
