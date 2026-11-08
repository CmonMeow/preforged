#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"

namespace fw::gw::skills::effects {
// Impact policy only. The cast and projectile must be scheduled separately;
// applying this at cast completion would interrupt before the captured impact.
struct DisruptingDaggerPlan {
    unsigned rank=0;
    double earth_damage=0;
    float range=624.f;
    bool operator==(const DisruptingDaggerPlan&) const = default;
};

inline std::optional<DisruptingDaggerPlan> disrupting_dagger(unsigned rank) {
    const auto* row=find_skill(571);
    if(rank>20||!row||row->profession!=7||row->attribute!=30||row->type!=5||
       row->energy_cost()!=5||row->activation!=0.25f||row->aftercast!=0.75f||
       row->recharge!=10||row->scale0!=10||row->scale15!=35||
       row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
    return DisruptingDaggerPlan{rank,damage_scale(10,35,rank),624.f};
}

// The damage is unconditional on a valid hostile impact. Only an actively
// activating skill is interruptible; an ordinary attack is not a skill.
inline bool disrupting_dagger_interrupts(unsigned active_skill) {
    return active_skill!=0&&find_skill(active_skill)!=nullptr;
}

struct DisruptingDaggerImpact {
    bool accepted=false;
    combat::Actor target;
    combat::Hit hit;
    bool interrupt_candidate=false;
};

// Pure impact proposal. World must provide the target's current, scoped
// combat/armor state and the skill actually activating at impact time. World
// still owns cancellation, protection and publication as one transaction.
inline DisruptingDaggerImpact disrupting_dagger_impact(
        const DisruptingDaggerPlan& plan,combat::Actor target,
        unsigned caster_level,unsigned active_skill,bool hostile){
    const auto canonical=disrupting_dagger(plan.rank);
    if(!canonical||plan!=*canonical||!target.generation)return {};
    const auto hit=combat::apply(target,
        {combat::Kind::earth,plan.earth_damage,caster_level,571},hostile);
    if(hit.reason!=combat::Reason::accepted)return {};
    return {true,target,hit,disrupting_dagger_interrupts(active_skill)};
}
}
