#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_cast_timing.h"
#include <algorithm>
#include <cmath>
#include <compare>
#include <optional>
#include <set>
#include <vector>

namespace fw::gw::skills::effects {

struct LightbringersGazePlan {
    unsigned rank=0,holy_damage=100,additional_foes=0;
    unsigned range=1248,area=312,disable_ms=15000;
    auto operator<=>(const LightbringersGazePlan&)const=default;
};

inline std::optional<LightbringersGazePlan> lightbringers_gaze(unsigned rank){
    const auto* row=find_skill(1814);
    if(rank>8||!row||row->campaign!=3||row->profession||row->attribute!=51||
       row->type!=10||row->energy_cost()!=5||row->activation!=1.f||
       row->aftercast!=.75f||row->recharge!=15||row->target!=5||
       row->scale0!=100||row->scale15!=100||row->caster_body_resource!=2077||
       row->target_body_resource!=2077)return {};
    return LightbringersGazePlan{rank,100,rank};
}

// The Gaze skill disable follows its recharge after completion-time recharge
// modifiers. Reuse the ordinary timer calculation; do not keep an independent
// fixed 15-second lockout when Serpent's Quickness or a proven Quickening
// Zephyr aura is active. World spirit lifetime/range authority remains a gate.
inline std::optional<Time> lightbringers_gaze_disable_duration(
        const LightbringersGazePlan& plan,
        const std::vector<Entry>& active,Time now,
        bool quickening_zephyr=false){
    if(lightbringers_gaze(plan.rank)!=plan)return {};
    const auto* row=find_skill(1814);
    if(!row)return {};
    const auto seconds=timing::recharge(*row,0,0,false,active,now,
                                        row->recharge,1.,quickening_zephyr);
    if(!seconds)return {};
    return Time(*seconds)*1000;
}

// The archived Guild Wars Wiki Lightbringer page states that all monsters in
// the Domain of Anguish qualify. Explicit reviewed NPC exceptions remain
// authoritative; elsewhere, an absent trait is still unknown.
inline std::optional<bool> lightbringer_servant_trait(
        unsigned map,std::optional<bool> reviewed){
    if(reviewed)return reviewed;
    if(map==474||map==445)return true;
    return {};
}

struct GazeCandidate {
    Scope scope;
    float distance_from_primary=0;
    bool alive=false,hostile=false;
    std::optional<bool> servant_of_abaddon;
};

// Primary eligibility is a reviewed taxonomy fact. Additional in-area foes
// need not be servants. Distance/actor ordering makes simultaneous picks
// deterministic without claiming the retail game's exact tie-break policy.
inline std::optional<std::vector<Scope>> select_lightbringers_gaze(
        const LightbringersGazePlan& plan,Scope primary,
        std::vector<GazeCandidate> candidates){
    if(lightbringers_gaze(plan.rank)!=plan||!primary.instance||!primary.actor||
       !primary.generation)return {};
    const auto selected=std::find_if(candidates.begin(),candidates.end(),
        [&](const auto& candidate){return candidate.scope==primary;});
    if(selected==candidates.end()||!selected->alive||!selected->hostile||
       selected->servant_of_abaddon!=std::optional(true))return {};
    std::set<Scope> seen{primary};
    std::vector<GazeCandidate> extras;
    for(const auto& candidate:candidates){
        if(candidate.scope==primary||candidate.scope.instance!=primary.instance||
           !candidate.scope.actor||!candidate.scope.generation||
           !candidate.alive||!candidate.hostile||
           !std::isfinite(candidate.distance_from_primary)||
           candidate.distance_from_primary<0||
           candidate.distance_from_primary>plan.area||
           !seen.insert(candidate.scope).second)continue;
        extras.push_back(candidate);
    }
    std::sort(extras.begin(),extras.end(),[](const auto& a,const auto& b){
        if(a.distance_from_primary!=b.distance_from_primary)
            return a.distance_from_primary<b.distance_from_primary;
        return a.scope<b.scope;
    });
    std::vector<Scope> result{primary};
    for(const auto& candidate:extras){
        if(result.size()>plan.additional_foes)break;
        result.push_back(candidate.scope);
    }
    return result;
}

struct GazeImpact {
    Scope target;
    unsigned holy_damage=0;
    bool interrupt=false;
    auto operator<=>(const GazeImpact&)const=default;
};

// The archived Guild Wars Wiki explicitly limits the interrupt to the
// demonic primary. Additional foes receive damage even if non-demonic.
inline std::optional<std::vector<GazeImpact>> lightbringers_gaze_impacts(
        const LightbringersGazePlan& plan,Scope primary,
        std::vector<GazeCandidate> candidates){
    auto targets=select_lightbringers_gaze(plan,primary,std::move(candidates));
    if(!targets)return {};
    std::vector<GazeImpact> impacts;
    impacts.reserve(targets->size());
    for(const auto& target:*targets)
        impacts.push_back({target,plan.holy_damage,target==primary});
    return impacts;
}
}
