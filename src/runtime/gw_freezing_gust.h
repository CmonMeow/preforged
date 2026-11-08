#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>

namespace fw::gw::skills::effects {
struct FreezingGustPlan {
    unsigned rank=0;double damage=20;std::uint64_t snare_ms=1000;
    float range=1248.f,multiplier=.34f;
    auto operator<=>(const FreezingGustPlan&)const=default;
};
inline std::optional<FreezingGustPlan> freezing_gust(unsigned rank){
    const auto* row=find_skill(1382);
    if(rank>20||!row||row->profession!=6||row->attribute!=11||row->type!=4||
       row->energy_cost()!=10||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=8||
       row->scale0!=20||row->scale15!=80||row->bonus0!=66||row->bonus15!=66||
       row->duration0!=1||row->duration15!=5||row->caster_body_resource!=2077||
       row->target_body_resource!=1643)return {};
    return FreezingGustPlan{rank,damage_scale(20,80,rank),
        std::uint64_t(interpolate(1,5,rank))*1000};
}
struct FreezingGustResult {
    bool accepted=false,damaged=false;combat::Actor actor;std::optional<combat::Hit> hit;
    std::uint64_t snare_ms=0;
};
// The target's Water Magic hex state is sampled when the spell completes.
// A successful damage branch creates no Freezing Gust snare; the other
// branch creates no damage. World owns scope, hex prevention and duration.
inline FreezingGustResult freezing_gust_target(const FreezingGustPlan& plan,
        combat::Actor actor,const combat::conditions::State& conditions,
        unsigned level,bool water_hexed,std::uint64_t now){
    const auto expected=freezing_gust(plan.rank);
    if(!expected||*expected!=plan||!actor.health||!combat::valid(*actor.health)||
       actor.health->current<=0||!actor.damageable||!level||level>255)return {};
    if(!water_hexed)return {true,false,actor,{},plan.snare_ms};
    if(!actor.cold_armor)return {};
    actor.cold_armor=conditions.armor(*actor.cold_armor,now);
    const auto hit=combat::apply(actor,{combat::Kind::cold,plan.damage,level,1382},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    return {true,true,actor,hit,0};
}
}
