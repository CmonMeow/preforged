#pragma once
#include "gw_ash_blast.h"
#include <set>
#include <span>

namespace fw::gw::skills::effects {

// Detached proposal boundary for one Ash Blast impact. World supplies only
// authoritative, already-selected hostile actors and an effect ID reserved in
// each target's own registry. Different targets may use the same numeric ID.
// No caller state is changed until every recipient has passed preflight.
struct AshBlastRecipient {
    Scope scope;
    unsigned effect=0;
    float distance_from_center=0;
    Registry registry;
    combat::conditions::State conditions;
    combat::Actor actor;
    bool hostile=false;
};
struct AshBlastBatch {
    Scope center;
    std::vector<std::pair<Scope,AshBlastTarget>> targets;
};
inline std::optional<AshBlastBatch> prepare_ash_blast_batch(
        const AshBlastPlan& plan,Scope caster,Scope center,
        std::span<const AshBlastRecipient> recipients,unsigned caster_level,
        Time now){
    if(!ash_blast(plan.rank)||*ash_blast(plan.rank)!=plan||
       !ash_scope(caster)||!ash_scope(center)||caster.instance!=center.instance||
       caster.actor==center.actor||!caster_level||caster_level>255||
       recipients.empty()||recipients.size()>4096)return {};
    AshBlastBatch batch{center,{}};
    batch.targets.reserve(recipients.size());
    std::set<unsigned> actors;
    bool found_center=false;
    for(const auto& recipient:recipients){
        if(!ash_scope(recipient.scope)||recipient.scope.instance!=caster.instance||
           recipient.scope.actor==caster.actor||
           !actors.insert(recipient.scope.actor).second||!recipient.effect||
           !std::isfinite(recipient.distance_from_center)||
           recipient.distance_from_center<0||
           recipient.distance_from_center>plan.radius||!recipient.hostile)return {};
        if(recipient.scope==center){
            if(recipient.distance_from_center!=0)return {};
            found_center=true;
        }
        auto proposed=prepare_ash_blast_target(plan,caster,recipient.scope,
            recipient.effect,now,recipient.registry,recipient.conditions,
            recipient.actor,caster_level,recipient.hostile,
            recipient.distance_from_center);
        if(!proposed)return {};
        batch.targets.emplace_back(recipient.scope,std::move(*proposed));
    }
    return found_center?std::optional(std::move(batch)):std::nullopt;
}

}
