#pragma once
#include "gw_skill_catalog_generated.h"
#include <cmath>
#include <optional>
namespace fw::gw::skills::effects {
struct LifeDrainHexPlan {
    unsigned skill,rank,pips;
    std::uint64_t duration;
    float range=1248.f,radius=0,break_range=0;
};
inline std::optional<LifeDrainHexPlan> life_drain_hex(unsigned skill,unsigned rank=0){
    if(rank>20||(skill!=109&&skill!=126))return {};
    const auto* row=find_skill(skill);if(!row||row->type!=4||row->attribute!=4)return {};
    const auto pips=static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.));
    const auto seconds=static_cast<unsigned>(std::lround(skill==109?12.+12.*rank/15.:6.+6.*rank/15.));
    // GWCA Constants::Range::Adjacent=166 and Compass=5000. Only Life Siphon
    // has the documented compass-distance termination; Transfer has none.
    return LifeDrainHexPlan{skill,rank,pips,seconds*1000u,1248.f,skill==126?166.f:0.f,skill==109?5000.f:0.f};
}
inline bool valid_life_drain_hex(const LifeDrainHexPlan& p){
    const auto x=life_drain_hex(p.skill,p.rank);return x&&p.pips==x->pips&&p.duration==x->duration&&p.range==x->range&&p.radius==x->radius&&p.break_range==x->break_range;
}
}
