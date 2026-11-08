#pragma once
#include "gw_ward_foes_route.h"
#include "gw_npc_combat_catalog.h"

namespace fw::gw::skills::effects {
// The caller supplies only allegiance-eligible wards and navigation-validated
// legs. An actor's retained movement history owns spell, aftercast, knockdown
// and ordinary movement effects; the ward factor joins before speed clamps.
inline std::optional<WardFoeRouteTravel> ward_foes_npc_route_travel(
    const WardFoeRecipient& recipient,
    std::span<const WardAgainstFoes> eligible_wards,
    std::span<const WardFoeRouteLeg> route,
    const combat::npcs::LiveActor& actor,
    const combat::npcs::Lifetime& lifetime,
    Time from, Time through, double base_speed){
    constexpr Time exact_time_limit=9007199254740991ULL;
    if(through<from||through>exact_time_limit||!std::isfinite(base_speed)||
       base_speed<=0||base_speed>1000000||recipient.scope!=actor.hex_scope())return {};
    const auto boundaries=actor.historical_movement_boundaries(lifetime,from,through);
    if(!boundaries)return {};
    std::vector<double> times;times.reserve(boundaries->size());
    for(const auto at:*boundaries)times.push_back(double(at));
    return ward_foes_route_travel(recipient,eligible_wards,route,
        double(from),double(through),times,
        [&](double at,float factor)->std::optional<double>{
            const auto multiplier=actor.historical_movement_rate(
                lifetime,static_cast<Time>(std::floor(at)),factor);
            if(!multiplier)return {};
            return base_speed * *multiplier;
        });
}
}
