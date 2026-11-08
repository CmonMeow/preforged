#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
// Pinned299 scale3..18 reduction and bonus5..50 healing; local wiki duration8s.
inline unsigned shielding_hands_reduction(const Registry& registry,Scope target,Time now){
    const auto entry=registry.newest_skill(target,299,now);const auto* row=find_skill(299);
    return entry&&row?interpolate(row->scale0,row->scale15,entry->rank):0;
}
}
