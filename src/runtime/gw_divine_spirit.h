#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_aura_restoration.h"
namespace fw::gw::skills::effects {
struct DivineSpiritPlan {
    unsigned rank=0,reduction=5,minimum=1;
    Time duration=1000,activation=250,aftercast=750,recharge=60000;
    auto operator<=>(const DivineSpiritPlan&)const=default;
};
inline std::optional<DivineSpiritPlan> divine_spirit(unsigned rank){
    const auto* row=find_skill(310);
    if(rank>20||!row||row->profession!=3||row->attribute!=16||row->type!=6||row->special||row->energy_cost()!=10||row->health_sacrifice||row->adrenaline||
       row->activation!=.25f||row->aftercast!=.75f||row->recharge!=60||row->duration0!=1||row->duration15!=14||row->scale0!=5||row->scale15!=5||row->bonus0!=1||row->bonus15!=1||row->caster_body_resource!=553||row->target_body_resource!=2077)return {};
    return DivineSpiritPlan{rank,5,1,Time(interpolate(1,14,rank))*1000};
}
// Standalone modifier only. This does not establish its ordering relative to
// Glyphs or other cost modifiers and is not yet wired into paid casting.
inline std::optional<unsigned> divine_spirit_cost(const SkillRecord& skill,unsigned cost,const Registry& registry,Scope caster,Time now){
    if(!caster.instance||!caster.actor||caster.actor>65535||!caster.generation||!registry.matches(caster))return {};
    const auto effect=registry.newest_skill(caster,310,now);
    if(!effect)return cost;
    const auto plan=divine_spirit(effect->rank);
    if(!plan||effect->cause!=caster.actor||effect->kind!=Category::enchantment||!effect->expires)return {};
    if(skill.profession!=3||!is_spell(skill))return cost;
    // No reduced-cost capture exists for zero-cost monster enchantments.
    // Keep that case explicit instead of inventing a cost increase or free cast.
    if(!cost)return {};
    return std::max(plan->minimum,cost>plan->reduction?cost-plan->reduction:0u);
}
}
