#pragma once
#include "gw_combat.h"
#include "gw_conditions.h"
#include "gw_skill_effect_plan.h"
#include "gw_deep_wound.h"
#include <algorithm>
#include <cmath>
#include <compare>
#include <optional>

namespace fw::gw::skills::effects {
struct FinishHimPlan {
    unsigned skill=2353,norn_rank=0,damage=44;
    std::uint64_t conditions_ms=12000;
    float range=1248;
    auto operator<=>(const FinishHimPlan&)const=default;
};
inline std::optional<FinishHimPlan> finish_him(unsigned norn_rank){
    const auto* row=find_skill(2353);
    if(norn_rank>10||!row||row->campaign!=4||row->profession||row->attribute!=51||
       row->type!=15||row->special!=524288||row->energy_cost()!=10||
       row->activation||row->aftercast||row->recharge!=15||
       row->scale0!=44||row->scale15!=80||row->duration0!=12||row->duration15!=20||
       row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
    // Norn ranks 0..5 map to effective attributes 0,3,6,9,12,15;
    // ranks 6..10 retain the rank-5 effect. Guild Wars Wiki progression.
    static constexpr unsigned damage_by_rank[]{44,51,58,66,73,80};
    static constexpr unsigned duration_by_rank[]{12,14,15,17,18,20};
    const auto step=std::min(norn_rank,5u);
    return FinishHimPlan{2353,norn_rank,damage_by_rank[step],
        1000ull*duration_by_rank[step]};
}
struct FinishHimResult {
    bool triggered=false;
    combat::Actor target;
    combat::conditions::State conditions;
    std::optional<combat::Hit> hit;
};
// Copy-only proposal. World must validate scoped hostility/range and replay
// condition reactions before damage through its shared authorities.
inline std::optional<FinishHimResult> resolve_finish_him(
        const FinishHimPlan& plan,combat::Actor actor,
        combat::conditions::State conditions,unsigned level,unsigned caster,
        std::uint64_t now,bool allows_conditions,std::optional<unsigned> threshold_capacity={}){
    if(finish_him(plan.norn_rank)!=plan||!caster||caster>65535||!level||level>255||
       !actor.id||actor.id>65535||!actor.health||!combat::valid(*actor.health)||
       actor.health->current<=0||!actor.damageable||
       (threshold_capacity&&(!*threshold_capacity||*threshold_capacity>actor.health->maximum)))return {};
    FinishHimResult result{false,actor,conditions,{}};
    if(actor.health->current*2>=threshold_capacity.value_or(actor.health->maximum))return result;
    const bool already_wounded=result.conditions.has(combat::conditions::Kind::deep_wound,now);
    if(allows_conditions){
        if(!result.conditions.apply(combat::conditions::Kind::cracked_armor,caster,now,plan.conditions_ms)||
           !result.conditions.apply(combat::conditions::Kind::deep_wound,caster,now,plan.conditions_ms))return {};
        if(!already_wounded){
            combat::deep_wound::State wound;wound.reset(result.target.health->maximum);
            auto health=result.target.health->current;
            wound.capacity(health,result.target.health->maximum,true);
            result.target.health=combat::Health{health,wound.maximum()};
        }
    }
    const auto hit=combat::apply(result.target,{combat::Kind::armor_ignoring,
                                      double(plan.damage),level,2353},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    result.triggered=true;result.hit=hit;
    return result;
}
}
