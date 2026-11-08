#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_aura_restoration.h"
#include "gw_power_block.h"
#include "gw_disrupting_dagger.h"
#include <compare>
namespace fw::gw::skills::effects {
struct InterruptPlan {unsigned skill=0,rank=0,damage=0,energy=0;bool any_action=false,energy_spell_only=false;float range=1248;unsigned target_loss=0,target_gain=0;bool hex_allows_skill=false,any_skill=false;auto operator<=>(const InterruptPlan&)const=default;};
inline std::optional<InterruptPlan> interrupt_skill(unsigned skill,unsigned rank=0){
    if(rank>20)return {};
    if(skill==5){if(!power_block(rank))return {};return InterruptPlan{skill,rank};}
    if(skill==23)return InterruptPlan{skill,rank,interpolate(30,120,rank),0};
    if(skill==25)return InterruptPlan{skill,rank,0,interpolate(1,31,rank)};
    if(skill==61)return InterruptPlan{skill,rank,0,interpolate(3,15,rank),true,true};
    if(skill==24){InterruptPlan p{skill,rank};p.target_loss=interpolate(3,17,rank);return p;}
    if(skill==931){InterruptPlan p{skill,rank};p.target_gain=interpolate(10,5,rank);return p;}
    if(skill==860){InterruptPlan p{skill,rank,interpolate(10,51,rank)};p.hex_allows_skill=true;return p;}
    return {};
}
// This plan is only for the already-landed Dagger projectile. Keeping it out
// of interrupt_skill() prevents Runtime from binding a completion-time cast.
inline std::optional<InterruptPlan> disrupting_dagger_interrupt_plan(unsigned rank){
    if(!disrupting_dagger(rank))return {};
    InterruptPlan plan{571,rank};plan.range=624.f;plan.any_skill=true;return plan;
}
inline bool valid_interrupt(const InterruptPlan& plan){
    const auto p=plan.skill==571?disrupting_dagger_interrupt_plan(plan.rank):interrupt_skill(plan.skill,plan.rank);
    return p&&*p==plan;
}
inline bool interrupt_matches(const InterruptPlan& plan,unsigned active_skill,bool ordinary_attack,bool hexed=false){
    const auto* row=find_skill(active_skill);
    if(plan.hex_allows_skill)return valid_interrupt(plan)&&row&&active_skill&&(is_spell(*row)||hexed);
    if(plan.any_skill)return valid_interrupt(plan)&&row&&active_skill;
    return valid_interrupt(plan)&&(plan.any_action?(ordinary_attack||active_skill!=0):(row&&(is_spell(*row)||row->type==27)));
}
}
