#pragma once
#include "gw_effect_registry.h"
#include <cmath>

namespace fw::gw::combat::ai {
// Hostile pursuit memory: acquisition distance is deliberately independent of
// continued pursuit. Distances are host-authored until captures measure them.
struct Pursuit {
    skills::effects::Scope source,target;
    float origin_x=0,origin_y=0;
    unsigned plane=0;
    bool returning=false;
};
inline bool retains(const Pursuit& memory,skills::effects::Scope source,
    skills::effects::Scope target,float x,float y,unsigned plane,
    float target_distance,float follow_range,float leash_range){
    return !memory.returning&&memory.source==source&&memory.target==target&&memory.plane==plane&&
        std::isfinite(x)&&std::isfinite(y)&&std::isfinite(target_distance)&&
        std::isfinite(follow_range)&&std::isfinite(leash_range)&&
        target_distance>=0&&follow_range>0&&leash_range>0&&
        target_distance<=follow_range&&
        std::hypot(x-memory.origin_x,y-memory.origin_y)<=leash_range;
}
}

