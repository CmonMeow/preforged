#pragma once
#include "gw_npc_combat_catalog.h"
#include "gw_parasitic_bond.h"
#include <set>
#include <span>
namespace fw::gw::skills::effects {
enum class BondBoundaryKind {npc_death,expiry};
struct BondBoundary {Time at=0;Scope target;unsigned effect=0;BondBoundaryKind kind=BondBoundaryKind::expiry;};
enum class BondForecastReason {okay,link,lifetime,clock,health,effect};
struct BondForecast {BondForecastReason reason=BondForecastReason::okay;std::optional<BondBoundary> next;};
// Lookup returns an optional VALUE snapshot for an NPC target,or null for a
// player target. No live actor is mutated. Forecast only up to the earliest
// return deadline: processing that return can change subsequent lifetimes.
// Death precedes expiry at equal timestamps,an explicit private-server tie
// policy rather than a claim about retail simultaneous-event ordering.
template<class Lookup> BondForecast forecast_parasitic_npcs(std::span<const ParasiticBonds::Link> links,Time horizon,Lookup&& lookup){
    BondForecast out;Time stop=horizon;
    const auto earlier=[](const BondBoundary& a,const BondBoundary& b){
        return std::tie(a.at,a.kind,a.target,a.effect)<std::tie(b.at,b.kind,b.target,b.effect);
    };
    const auto select=[&](BondBoundary candidate){if(!out.next||earlier(candidate,*out.next))out.next=candidate;};
    for(const auto& link:links){
        const auto plan=parasitic_bond(link.rank);
        if(!plan||!link.effect||!link.caster.instance||!link.caster.actor||!link.caster.generation||
           !link.target.actor||!link.target.generation||link.caster.instance!=link.target.instance||
           link.caster==link.target||link.applied>UINT64_MAX-plan->duration||link.expires!=link.applied+plan->duration)
            return {BondForecastReason::link,{}};
        if(link.expires<=horizon){select({link.expires,link.target,link.effect,BondBoundaryKind::expiry});stop=std::min(stop,link.expires);}
    }
    std::set<Scope> visited;
    for(const auto& link:links){
        if(link.applied>stop)continue;
        auto snapshot=lookup(link.target);if(!snapshot)continue;
        const auto life=snapshot->lifetime();
        if(snapshot->hex_scope()!=link.target)return {BondForecastReason::lifetime,{}};
        const auto entry=snapshot->hex_effect(life,link.effect);
        if(!entry||entry->skill!=99||entry->cause!=link.caster.actor||entry->rank!=link.rank||
           entry->applied!=link.applied||entry->expires!=link.expires)return {BondForecastReason::effect,{}};
        if(!visited.insert(link.target).second)continue;
        if(const auto clock=snapshot->health_clock(life);clock&&*clock>stop)return {BondForecastReason::clock,{}};
        const auto* actor=snapshot->actor(life);
        if(!actor||!actor->health||actor->health->current<=0)return {BondForecastReason::health,{}};
        const auto update=snapshot->advance_conditions(life,stop);
        if(update&&update->killed)select({update->death_time,link.target,link.effect,BondBoundaryKind::npc_death});
    }
    return out;
}
}
