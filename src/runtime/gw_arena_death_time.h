#pragma once
#include "gw_arena_match.h"
namespace fw::gw::arena {
// A single authoritative World snapshot supplies both corpse timestamps.
// Connection polling order must not choose the first-death return deadline.
inline unsigned record_deaths(MatchLifecycle& match,const std::array<std::uint32_t,2>& players,
        const std::array<std::optional<std::uint64_t>,2>& deaths){
    const unsigned first=deaths[1]&&(!deaths[0]||*deaths[1]<*deaths[0])?1:0;
    unsigned accepted=0;
    for(const unsigned index:{first,1-first})
        if(deaths[index]&&match.death(players[index],*deaths[index]))++accepted;
    return accepted;
}
}
