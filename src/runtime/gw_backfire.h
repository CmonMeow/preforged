#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_aura_restoration.h"
#include <compare>
namespace fw::gw::skills::effects {
struct BackfirePlan {
    unsigned skill=28,rank=0,damage=35;
    Time duration=10000;
    float range=1248;
    auto operator<=>(const BackfirePlan&)const=default;
};
inline std::optional<BackfirePlan> backfire(unsigned skill,unsigned rank=0){
    if(skill!=28||rank>20)return {};
    return BackfirePlan{skill,rank,interpolate(35,140,rank)};
}
inline bool valid_backfire(const BackfirePlan& plan){const auto expected=backfire(plan.skill,plan.rank);return expected&&*expected==plan;}
// Called after successful spell activation, BEFORE any spell effect, including
// healing and removal of this hex. Skills/signets/attacks do not trigger it.
inline std::optional<Entry> backfire_reaction(const Registry& registry,Scope scope,unsigned cast_skill,Time now){
    const auto* spell=find_skill(cast_skill);
    return spell&&is_spell(*spell)?registry.newest_skill(scope,28,now):std::nullopt;
}
}
