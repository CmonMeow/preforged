#pragma once
#include "gw_skill_catalog_generated.h"
#include "gw_combat.h"
#include <compare>
#include <optional>

namespace fw::gw::skills::effects {
// The captured cast proves the ordinary 30-second recharge and damage wire.
// The corpse/dead-pet shortcut comes from the local Guild Wars Wiki copy and
// remains a separate branch until a retail packet trace exercises it.
struct SignetSorrowPlan {
    unsigned skill=1363,rank=0,damage=15;
    unsigned activation_ms=1000,aftercast_ms=750,recharge_ms=30000;
    float range=1248,corpse_radius=252;
    auto operator<=>(const SignetSorrowPlan&)const=default;
};

inline std::optional<SignetSorrowPlan> signet_sorrow(unsigned rank){
    const auto* row=find_skill(1363);
    if(rank>20||!row||row->campaign!=3||row->profession!=4||row->attribute!=6||
       row->type!=7||row->special!=0||row->energy_cost()!=0||row->health_sacrifice||
       row->adrenaline||row->activation!=1.f||row->aftercast!=.75f||
       row->recharge!=30||row->scale0!=15||row->scale15!=75||
       row->caster_body_resource!=1619||row->target_body_resource!=2077)return {};
    SignetSorrowPlan plan;plan.rank=rank;plan.damage=15+4*rank;return plan;
}

struct SignetSorrowFacts {
    bool target_is_foe=false,target_alive=false,dead_pet=false;
    float target_distance=0;
    // Distance from the target to the nearest authoritative available corpse.
    // Absence must remain unknown rather than being treated as a corpse.
    std::optional<float> nearest_corpse_distance;
};
struct SignetSorrowHit {combat::Damage damage;unsigned recharge_ms=30000;};



// Pure preflight only. World must prove target allegiance, current position,
// corpse/pet lifetime and Health immediately before committing the hit.
inline std::optional<SignetSorrowHit> prepare_signet_sorrow(
        const SignetSorrowPlan& plan,unsigned level,const SignetSorrowFacts& facts){
    if(signet_sorrow(plan.rank)!=plan||!level||level>255||
       !facts.target_is_foe||!facts.target_alive||
       !std::isfinite(facts.target_distance)||facts.target_distance<0||
       facts.target_distance>plan.range||
       (facts.nearest_corpse_distance&&(!std::isfinite(*facts.nearest_corpse_distance)||
           *facts.nearest_corpse_distance<0)))return {};
    combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage),level,plan.skill};
    if(!combat::valid(damage))return {};
    return SignetSorrowHit{damage,
        (facts.nearest_corpse_distance&&*facts.nearest_corpse_distance<=plan.corpse_radius)||
            facts.dead_pet?0u:plan.recharge_ms};
}
}
