#pragma once
#include "gw_skill_catalog_generated.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include "gw_wound_spells.h"
#include <compare>
namespace fw::gw::skills::effects {
struct ConditionDamagePlan {
    unsigned skill,rank,damage,pips;
    combat::Kind kind;
    std::uint64_t duration;
    float range=1248.f,radius=0;
    auto operator<=>(const ConditionDamagePlan&)const=default;
};
inline std::optional<ConditionDamagePlan> condition_damage(unsigned skill,unsigned rank=0){
    if(rank>20||(skill!=828&&skill!=864))return {};
    const auto* row=find_skill(skill);if(!row)return {};
    const auto scale=[&](unsigned a,unsigned b){return static_cast<unsigned>(std::lround(a+(double(b)-a)*rank/15.));};
    return ConditionDamagePlan{skill,rank,scale(row->scale0,row->scale15),skill==828?scale(row->bonus0,row->bonus15):0u,
        skill==828?combat::Kind::cold:combat::Kind::armor_ignoring,
        skill==828?10000u:scale(row->bonus0,row->bonus15)*1000u,1248.f,skill==864?166.f:0.f};
}
// Reuse the ordered damage/conditional-recipient authority without changing
// Runtime's wound-family dispatch or enabling Phantom Pain's end lifecycle.
inline std::optional<ConditionDamagePlan> accumulated_pain_damage(unsigned skill,unsigned rank=0){
    const auto plan=wound_spell(skill,rank);if(!plan||plan->phantom)return {};
    return ConditionDamagePlan{skill,rank,plan->damage,0,combat::Kind::armor_ignoring,
        plan->wound_duration,plan->range,0};
}
inline bool valid_condition_damage(const ConditionDamagePlan& p){
    const auto x=condition_damage(p.skill,p.rank),wound=accumulated_pain_damage(p.skill,p.rank);
    return (x&&*x==p)||(wound&&*wound==p);
}
}
