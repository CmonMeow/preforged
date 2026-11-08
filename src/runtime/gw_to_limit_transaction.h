#pragma once
#include "gw_temporary_health.h"
#include "gw_adrenaline.h"
#include "gw_for_great_justice.h"
#include "gw_control_hexes.h"
namespace fw::gw::skills::effects {
struct ToLimitProposal {
    TemporaryHealthTransition health;
    Registry registry;
    combat::adrenaline::State adrenaline;
    Entry applied;
    bool gain_forbidden=false;
};
// Copy-only full effect preparation. The caller supplies an authoritative foe
// count at completion, then commits all three authorities together. Payment,
// cooldown and wire publication belong to the normal instant-shout lifecycle.
inline std::optional<ToLimitProposal> prepare_to_limit(const TemporaryHealth& health,
        TemporaryHealthSnapshot snapshot,const Registry& registry,
        const combat::adrenaline::State& adrenaline,const ToLimitPlan& plan,
        unsigned effect,unsigned reviewed_foes,bool wounded){
    const auto owner=snapshot.owner;const auto now=snapshot.at;
    if(!registry.matches(owner)||snapshot.current<=0||adrenaline.cursor()>now)return {};
    const auto result=to_limit_result(plan,reviewed_foes);if(!result)return {};
    // A ledger and status registry must describe the same owned source.
    const auto existing=registry.newest_skill(owner,316,now);
    if(health.source()){
        const auto& source=*health.source();const auto entry=registry.lookup(owner,source.effect);
        if(!existing||existing->instance!=source.effect||!entry||entry->cause!=owner.actor||
           entry->rank!=source.rank||entry->applied!=source.begins||entry->expires!=source.ends)return {};
    }else if(existing)return {};
    const auto next=prepare_to_limit_health(health,snapshot,plan,effect,wounded);if(!next)return {};
    auto statuses=registry;const Entry applied{effect,owner.actor,316,plan.rank,Category::other,now,now+plan.duration};
    // Never replace another producer's effect identity.
    if(registry.lookup(owner,effect)||!statuses.apply(owner,applied,Replace::same_skill).accepted)return {};
    auto pools=adrenaline;
    const bool forbidden=adrenaline_forbidden(registry.active(owner,now),now);
    if(result->adrenaline_units&&!forbidden)
        pools.gain_halves(std::uint64_t(result->adrenaline_units)*adrenaline_half_rate(registry,owner,now),now);
    return ToLimitProposal{*next,std::move(statuses),std::move(pools),applied,forbidden};
}
}
