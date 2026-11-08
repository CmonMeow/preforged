#pragma once
#include "gw_blocking.h"
#include <limits>

namespace fw::gw::skills::effects {
// Scoped registry transaction only. World still establishes hostile relation,
// live actors, range, resource costs and publication before exposing a cast.
inline std::optional<Applied> apply_rigor_mortis(Registry& registry,Scope target,unsigned effect,unsigned cause,
        const RigorMortisPlan& plan,Time now){
    const auto canonical=rigor_mortis(plan.rank);
    if(!canonical||*canonical!=plan||!target.instance||!target.actor||target.actor>65535||!target.generation||
       !effect||!cause||cause>65535||cause==target.actor||now>std::numeric_limits<Time>::max()-plan.duration)return {};
    auto result=registry.apply(target,{effect,cause,137,plan.rank,Category::hex,now,now+plan.duration},Replace::same_skill);
    return result.accepted?std::optional(std::move(result)):std::nullopt;
}
}
