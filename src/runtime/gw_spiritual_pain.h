#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <cmath>
#include <compare>
#include <optional>
#include <vector>

namespace fw::gw::skills::effects {
struct SpiritualPainPlan {
    unsigned rank=0,primary_damage=15,summoned_damage=25;
    float range=1248.f,area=322.f;
    auto operator<=>(const SpiritualPainPlan&)const=default;
};
inline std::optional<SpiritualPainPlan> spiritual_pain(unsigned rank){
    const auto* row=find_skill(1336);
    if(rank>20||!row||row->profession!=5||row->attribute!=2||row->type!=5||
       row->energy_cost()!=5||row->activation!=1.f||row->aftercast!=.75f||
       row->recharge!=7||row->scale0!=15||row->scale15!=75||
       row->bonus0!=25||row->bonus15!=125||
       row->caster_body_resource!=2077||row->target_body_resource!=1530)return {};
    return SpiritualPainPlan{rank,interpolate(15,75,rank),interpolate(25,125,rank)};
}
// The selected foe takes the primary hit. Every hostile summoned creature in
// the target's area takes the second hit, including the selected foe if it is
// itself a summoned creature. World supplies authoritative hostility, summon
// identity and distance; unknown summon identity must not be guessed.
inline std::optional<std::vector<combat::Hit>> spiritual_pain_target(
        const SpiritualPainPlan& plan,combat::Actor& actor,unsigned level,
        bool hostile,bool selected,std::optional<bool> summoned,float distance){
    if(spiritual_pain(plan.rank)!=plan||!level||level>255||!hostile||
       !summoned||!std::isfinite(distance)||distance<0||distance>plan.area||
       !actor.health||!combat::valid(*actor.health)||actor.health->current<=0||
       !actor.damageable)return {};
    std::vector<combat::Hit> hits;
    if(selected){
        const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,
            double(plan.primary_damage),level,1336},true);
        if(hit.reason!=combat::Reason::accepted)return {};
        hits.push_back(hit);
    }
    if(*summoned&&actor.health->current>0){
        const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,
            double(plan.summoned_damage),level,1336},true);
        if(hit.reason!=combat::Reason::accepted)return {};
        hits.push_back(hit);
    }
    return hits;
}
}
