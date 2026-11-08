#pragma once
#include "gw_skill_catalog_generated.h"
#include <cmath>
#include <optional>
#include <limits>
namespace fw::gw::skills::effects {
struct WoundSpellPlan {
    unsigned skill,rank,damage,degeneration;
    std::uint64_t hex_duration,wound_duration;
    float range=1248.f;
    bool phantom;
};
inline std::optional<WoundSpellPlan> wound_spell(unsigned skill,unsigned rank=0){
    if(rank>20||(skill!=44&&skill!=1052&&skill!=3184))return {};
    const auto* row=find_skill(skill);if(!row)return {};
    const auto scaled=static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.));
    // Phantom's native bonus is its end-condition lifetime; Accumulated's
    // native bonus2 is the required active-hex count, lifetime is wiki5..20.
    const auto duration=skill==44?static_cast<unsigned>(std::lround(row->bonus0+(double(row->bonus15)-row->bonus0)*rank/15.)):5u+rank;
    return WoundSpellPlan{skill,rank,skill==44?0u:scaled,skill==44?scaled:0u,skill==44?10000u:0u,duration*1000u,1248.f,skill==44};
}
inline bool valid_wound_spell(const WoundSpellPlan& p){
    const auto expected=wound_spell(p.skill,p.rank);return expected&&p.damage==expected->damage&&p.degeneration==expected->degeneration&&p.hex_duration==expected->hex_duration&&p.wound_duration==expected->wound_duration&&p.range==expected->range&&p.phantom==expected->phantom;
}
// Both boundaries must be representable before admitting Phantom Pain. A
// coarse resource update needs the second boundary as well: otherwise the
// newly applied Deep Wound can remain active past its actual expiration.
struct PhantomPainDeadlines {
    std::uint64_t hex_end,wound_end;
};
inline std::optional<PhantomPainDeadlines> phantom_pain_deadlines(
        const WoundSpellPlan& plan,std::uint64_t applied){
    if(!valid_wound_spell(plan)||!plan.phantom)return {};
    constexpr auto maximum=std::numeric_limits<std::uint64_t>::max();
    if(plan.hex_duration>maximum-applied)return {};
    const auto hex_end=applied+plan.hex_duration;
    if(plan.wound_duration>maximum-hex_end)return {};
    return PhantomPainDeadlines{hex_end,hex_end+plan.wound_duration};
}
}
