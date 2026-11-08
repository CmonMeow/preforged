#pragma once
#include "gw_attack_interrupts.h"
#include "gw_combat.h"
#include "gw_effect_registry.h"
#include <algorithm>
#include <cmath>
#include <compare>
#include <vector>

namespace fw::gw::skills::effects {

struct WanderingEyePlan {
    unsigned rank=0, damage=30;
    std::uint64_t duration=4000;
    // Archived skill description: nearby; supplied SDK Range::Nearby = 252.
    float range=1248.f, nearby=252.f;
    auto operator<=>(const WanderingEyePlan&) const = default;
};

inline std::optional<WanderingEyePlan> wandering_eye(unsigned rank){
    const auto* row=find_skill(2056);
    if(rank>20||!row||row->campaign!=4||row->profession!=5||
       row->attribute!=1||row->type!=4||row->special||
       row->energy_cost()!=5||row->activation!=2.f||
       row->aftercast!=.75f||row->recharge!=12||row->target!=5||
       row->scale0!=30||row->scale15!=110||
       row->duration0!=4||row->duration15!=4||
       row->caster_body_resource!=1716||row->target_body_resource!=1717)
        return {};
    return WanderingEyePlan{rank,interpolate(30,110,rank)};
}

struct WanderingEyeApplication {
    Registry registry;
    Entry entry;
    std::vector<Entry> replaced;
};

// Prepare the selected-recipient hex on a copied registry. World is responsible
// for range, allegiance, protection, the expiration timer and publication.
inline std::optional<WanderingEyeApplication> prepare_wandering_eye(
        const Registry& current,Scope caster,Scope target,
        std::uint32_t effect,const WanderingEyePlan& plan,Time now,Time duration){
    if(wandering_eye(plan.rank)!=plan||!current.matches(target)||
       !caster.instance||!caster.actor||!caster.generation||
       !target.instance||!target.actor||!target.generation||
       caster.instance!=target.instance||caster.actor==target.actor||
       !effect||!duration||now>UINT64_MAX-duration)return {};
    Registry proposed=current;
    Entry entry{effect,caster.actor,2056,plan.rank,Category::hex,now,now+duration};
    auto applied=proposed.apply(target,entry,Replace::same_skill);
    if(!applied.accepted)return {};
    return WanderingEyeApplication{std::move(proposed),entry,std::move(applied.removed)};
}

struct WanderingEyeConsumption {Entry entry;Registry registry;};

// A one-shot attack consumes the current hex identity before any area damage.
// Previewing the copy leaves the authoritative target intact on failure.
inline std::optional<WanderingEyeConsumption> consume_wandering_eye(
        const Registry& current,Scope target,Time now){
    const auto entry=current.newest_skill(target,2056,now);
    if(!entry||entry->kind!=Category::hex||!wandering_eye(entry->rank))return {};
    Registry proposed=current;
    if(!proposed.remove(target,entry->instance))return {};
    return WanderingEyeConsumption{*entry,std::move(proposed)};
}

// This only classifies a trusted action at its start. World must own the
// hex's one-use consumption, interrupt protection, nearby recipients and
// damage transaction; a client-selected target flag is not sufficient.
inline std::optional<bool> wandering_eye_attack_trigger(
        const WanderingEyePlan& plan,
        const combat::weapon::AttackActionFacts& action){
    if(wandering_eye(plan.rank)!=plan)return {};
    const auto classified=combat::weapon::classify_interrupt_action(action);
    if(!classified)return {};
    if(action.kind==combat::weapon::AttackTargetAction::attack)
        return classified->active;
    if(action.kind==combat::weapon::AttackTargetAction::skill&&action.skill){
        const auto* row=find_skill(action.skill->skill);
        return row&&row->type==14;
    }
    return false;
}

struct WanderingEyeCandidate {
    Scope actor;
    float distance_from_hexed=0;
    bool alive=false, same_plane=false;
    std::optional<bool> hostile_to_caster;
};

// A read-only area proposal for the attack-start transaction. Caller supplies
// independently reviewed allegiance, life and position at the event tick.
inline std::optional<std::vector<Scope>> wandering_eye_targets(
        const WanderingEyePlan& plan,Scope caster,Scope hexed,
        const std::vector<WanderingEyeCandidate>& population){
    if(wandering_eye(plan.rank)!=plan||!caster.instance||!caster.actor||
       !caster.generation||!hexed.actor||!hexed.generation||
       caster.instance!=hexed.instance||caster.actor==hexed.actor)return {};
    std::vector<Scope> targets;
    std::vector<Scope> seen;
    bool found_hexed=false;
    for(const auto& candidate:population){
        if(candidate.actor.instance!=hexed.instance||!candidate.actor.actor||
           !candidate.actor.generation||!std::isfinite(candidate.distance_from_hexed)||
           candidate.distance_from_hexed<0||
           std::find(seen.begin(),seen.end(),candidate.actor)!=seen.end())return {};
        seen.push_back(candidate.actor);
        if(candidate.actor==hexed)found_hexed=true;
        if(!candidate.alive||!candidate.same_plane||
           candidate.distance_from_hexed>plan.nearby)continue;
        if(!candidate.hostile_to_caster)return {};
        if(*candidate.hostile_to_caster)targets.push_back(candidate.actor);
    }
    if(!found_hexed||std::find(targets.begin(),targets.end(),hexed)==targets.end())return {};
    std::sort(targets.begin(),targets.end());
    return targets;
}

// Preflight the complete area against copied combat actors. The caller must
// commit all accepted hits in one World-locked transaction after validating
// generations again; this function never mutates authoritative Health.
inline std::optional<std::vector<combat::Hit>> wandering_eye_hits(
        const WanderingEyePlan& plan,unsigned caster_level,
        const std::vector<Scope>& targets,
        const std::vector<combat::Actor>& actors){
    if(wandering_eye(plan.rank)!=plan||!caster_level||caster_level>255||
       targets.empty()||targets.size()!=actors.size())return {};
    std::vector<combat::Hit> hits;
    hits.reserve(targets.size());
    for(std::size_t i=0;i<targets.size();++i){
        if(!targets[i].instance||!targets[i].actor||!targets[i].generation||
           actors[i].id!=targets[i].actor||
           actors[i].generation!=targets[i].generation||
           (i&&!(targets[i-1]<targets[i])))return {};
        auto preview=actors[i];
        const auto hit=combat::apply(preview,
            {combat::Kind::armor_ignoring,double(plan.damage),caster_level,2056},true);
        if(hit.reason!=combat::Reason::accepted)return {};
        hits.push_back(hit);
    }
    return hits;
}

}
