#pragma once
#include "gw_combat.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <compare>
#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace fw::gw::skills::effects {
struct AneurysmPlan {
    unsigned rank=0,damage_per_energy=1,adjacent_loss_cap=1;
    float range=1248.f,adjacent_radius=166.f;
    auto operator<=>(const AneurysmPlan&)const=default;
};
inline std::optional<AneurysmPlan> aneurysm(unsigned rank){
    const auto* row=find_skill(2055);
    if(rank>20||!row||row->campaign!=4||row->profession!=5||
       row->attribute!=2||row->type!=5||row->energy_cost()!=5||
       row->activation!=1.f||row->aftercast!=.75f||row->recharge!=5||
       row->target!=5||row->scale0!=1||row->scale15!=3||
       row->bonus0!=1||row->bonus15!=30||
       row->caster_body_resource!=1710||row->target_body_resource!=1711)return {};
    return AneurysmPlan{rank,interpolate(1,3,rank),interpolate(1,30,rank)};
}
struct AneurysmResult {
    unsigned restored_energy=0,damage_energy=0,adjacent_loss=0;
    std::optional<combat::Hit> hit;
};
// Copy-only target proposal. The caller supplies authoritative Energy and
// applies the returned primary/adjacent changes as one World transaction.
// The archived skill anomaly calculates generated Energy against the base
// maximum, even when Overcast prevents some of it from being received.
// Keep actual restoration separate from generated damage/adjacent loss.
inline std::optional<AneurysmResult> aneurysm_target(const AneurysmPlan& plan,
        combat::Actor target,unsigned current_energy,unsigned effective_max_energy,
        unsigned base_max_energy,unsigned caster_level){
    if(aneurysm(plan.rank)!=plan||!caster_level||caster_level>255||
       !target.health||!combat::valid(*target.health)||
       target.health->current<=0||!target.damageable||
       !base_max_energy||base_max_energy>1000000||
       effective_max_energy>base_max_energy||current_energy>base_max_energy)return {};
    const unsigned generated=base_max_energy-current_energy;
    const unsigned restored=effective_max_energy>current_energy?
        std::min(generated,effective_max_energy-current_energy):0;
    AneurysmResult result{restored,generated,
        std::min(generated,plan.adjacent_loss_cap),{}};
    if(!generated)return result;
    const auto damage=double(generated)*plan.damage_per_energy;
    const auto hit=combat::apply(target,
        {combat::Kind::armor_ignoring,damage,caster_level,2055},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    result.hit=hit;
    return result;
}
struct AneurysmNearby {
    combat::Actor actor;
    float distance=0;
    bool hostile=false;
    std::optional<unsigned> current_energy;
};
struct AneurysmLoss {
    std::uint32_t actor=0;
    std::uint64_t generation=0;
    unsigned energy=0;
};
struct AneurysmArea {
    AneurysmResult primary;
    std::vector<AneurysmLoss> adjacent;
};
// World supplies settled, scoped actors and authoritative hostility/distance.
// An eligible adjacent foe with unknown Energy rejects the whole proposal;
// omission must not silently become a zero-Energy target. No state is changed.
inline std::optional<AneurysmArea> aneurysm_area(const AneurysmPlan& plan,
        combat::Actor primary,unsigned current_energy,unsigned effective_max_energy,
        unsigned base_max_energy,unsigned caster_level,
        std::span<const AneurysmNearby> nearby){
    if(nearby.size()>4096||!primary.id||!primary.generation)return {};
    auto result=aneurysm_target(plan,primary,current_energy,
        effective_max_energy,base_max_energy,caster_level);
    if(!result)return {};
    AneurysmArea area{*result,{}};
    std::set<std::uint32_t> seen{primary.id};
    for(const auto& sample:nearby){
        if(!sample.actor.id||!sample.actor.generation||
           !seen.insert(sample.actor.id).second||
           !std::isfinite(sample.distance)||sample.distance<0)return {};
        if(!sample.hostile||sample.distance>plan.adjacent_radius)continue;
        if(!sample.actor.health||!combat::valid(*sample.actor.health)||
           sample.actor.health->current<=0||!sample.current_energy||
           *sample.current_energy>1000000)return {};
        area.adjacent.push_back({sample.actor.id,sample.actor.generation,
            std::min(*sample.current_energy,result->adjacent_loss)});
    }
    return area;
}
}
