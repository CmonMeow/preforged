#pragma once
#include "gw_effect_registry.h"
#include <cmath>
namespace fw::gw::skills::effects {
// Reviewed planner only. Attack dispatch must call the reaction once for each
// attack, including misses/blocks, and query reduction separately at impact.
// No cast handler or network success is implied by constructing this plan.
struct EmpathyPlan {unsigned skill,rank,damage,reduction;Time duration;};
inline std::optional<EmpathyPlan> empathy(unsigned skill,unsigned rank=0){
    if(rank>20||(skill!=26&&skill!=3151))return {};
    const auto* row=find_skill(skill);if(!row||row->type!=4||row->attribute!=2)return {};
    const auto scale=[&](unsigned low,unsigned high){return static_cast<unsigned>(std::lround(low+(double(high)-low)*rank/15.));};
    return EmpathyPlan{skill,rank,scale(row->scale0,row->scale15),scale(row->bonus0,row->bonus15),scale(5,15)*1000ull};
}
inline std::optional<EmpathyPlan> active_empathy(const Registry& registry,Scope target,Time now){
    // PvE/PvP versions are alternatives, never additive copies of one hex.
    const auto entry=registry.newest_skill(target,26,now,3151);
    return entry?empathy(entry->skill,entry->rank):std::nullopt;
}
inline std::optional<unsigned> empathy_attack_reaction(const Registry& registry,Scope target,Time now){
    const auto plan=active_empathy(registry,target,now);
    return plan?std::optional(plan->damage):std::nullopt;
}
inline std::optional<double> empathy_attack_damage(const Registry& registry,Scope attacker,Time impact,double damage){
    if(!std::isfinite(damage)||damage<0)return {};
    const auto plan=active_empathy(registry,attacker,impact);
    return std::max(0.,damage-(plan?plan->reduction:0));
}
}
