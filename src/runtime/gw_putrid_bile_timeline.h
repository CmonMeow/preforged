#pragma once
#include "gw_putrid_bile.h"
#include "gw_npc_combat_catalog.h"
#include <span>
#include <tuple>

namespace fw::gw::skills::effects {
enum class BileBoundaryKind {expiry,npc_death};
struct BileBoundary {
    Time at=0;Scope target;unsigned effect=0;
    BileBoundaryKind kind=BileBoundaryKind::expiry;
    auto operator<=>(const BileBoundary&)const=default;
};
enum class BileForecastReason {okay,link,lifetime,effect,clock,health,advance};
struct BileForecast {BileForecastReason reason=BileForecastReason::okay;std::optional<BileBoundary> next;};
// A missing lookup is a missing lifetime, never an implicit player/static NPC.
// Player Health forecasting remains the World's job. Its exact hex is still
// validated here, so a stale link cannot supply a false expiry frontier.
struct BileTimelineTarget {
    Scope scope;
    std::optional<combat::npcs::LiveActor> npc;
    std::optional<Entry> player_hex;
    std::optional<combat::npcs::LiveActor::ExternalHealth> external;
};

// Copy-only provider. World must first restrict horizon to all other event
// boundaries, including Energy-zero reactions, and supply the SAME external
// Health/Energy exposure used by actual NPC integration. This does not commit
// death, select explosion recipients, or reconstruct already discarded poses.
template<class Lookup>
BileForecast forecast_putrid_bile(std::span<const PutridBileLink> links,Time horizon,Lookup&& lookup){
    BileForecast out;Time stop=horizon;
    const auto select=[&](BileBoundary candidate){
        if(!out.next||candidate<*out.next)out.next=candidate;
    };
    std::vector<BileTimelineTarget> targets;targets.reserve(links.size());
    for(const auto& link:links){
        if(!putrid_bile(link.rank)||!link.effect||!link.caster.instance||!link.caster.actor||
           link.caster.actor>65535||!link.caster.generation||!link.target.actor||link.target.actor>65535||
           !link.target.generation||link.caster.instance!=link.target.instance||link.caster==link.target||
           !link.caster_level||link.caster_level>255||link.applied>=link.expires)
            return {BileForecastReason::link,{}};
        auto target=lookup(link.target);
        if(!target||target->scope!=link.target||(target->npc&&target->npc->hex_scope()!=link.target))
            return {BileForecastReason::lifetime,{}};
        const auto hex=target->npc?target->npc->hex_effect(target->npc->lifetime(),link.effect):target->player_hex;
        // Compare the actual admitted end time, not the unmodified plan duration.
        if(!hex||hex->instance!=link.effect||hex->kind!=Category::hex||hex->skill!=2058||
           hex->cause!=link.caster.actor||hex->rank!=link.rank||hex->applied!=link.applied||hex->expires!=link.expires)
            return {BileForecastReason::effect,{}};
        targets.push_back(std::move(*target));
        if(link.expires<=horizon){select({link.expires,link.target,link.effect,BileBoundaryKind::expiry});stop=std::min(stop,link.expires);}
    }
    for(std::size_t i=0;i<links.size();++i){
        const auto& link=links[i];auto& target=targets[i];
        if(link.applied>stop||!target.npc)continue;
        auto& snapshot=*target.npc;const auto life=snapshot.lifetime();
        if(snapshot.health_clock(life).value_or(stop)>stop||snapshot.energy_clock(life).value_or(stop)>stop)
            return {BileForecastReason::clock,{}};
        const auto state=snapshot.actor(life);
        if(!state||!state->health||state->health->current<=0)return {BileForecastReason::health,{}};
        const auto update=snapshot.advance_conditions(life,stop,target.external?&*target.external:nullptr);
        if(!update)return {BileForecastReason::advance,{}};
        // Death at the expiry instant is NOT a Bile explosion. take_death has
        // always required death < expires; preserve that ownership contract.
        if(update->killed&&link.applied<=update->death_time&&update->death_time<link.expires)
            select({update->death_time,link.target,link.effect,BileBoundaryKind::npc_death});
    }
    return out;
}
}
