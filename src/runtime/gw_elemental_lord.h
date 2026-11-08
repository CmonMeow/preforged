#pragma once
#include "gw_aura_restoration.h"
#include <algorithm>
#include <array>

namespace fw::gw::skills::effects {
struct ElementalLordPlan {
    unsigned skill=0,rank=0,duration_ms=40000,healing_percent=100;
    unsigned attribute_bonus=1,energy=5,activation_ms=250,aftercast_ms=750,recharge_ms=45000;
    unsigned caster_body=295,target_body=2077;
    bool operator==(const ElementalLordPlan&)const=default;
};
inline std::optional<ElementalLordPlan> elemental_lord(unsigned skill,unsigned rank){
    if((skill!=1951&&skill!=2094)||rank>12)return {};
    const auto* row=find_skill(skill);
    if(!row||row->campaign||row->profession!=6||row->attribute!=51||row->type!=6||
       row->special!=524288||row->energy_cost()!=5||row->health_sacrifice||row->overcast!=99||
       row->adrenaline||row->activation!=.25f||row->aftercast!=.75f||row->recharge!=45||
       row->target||row->scale0!=100||row->scale15!=300||row->bonus0!=1||row->bonus15!=1||
       row->duration0!=40||row->duration15!=60||row->caster_body_resource!=295||
       row->target_body_resource!=2077)return {};
    // Published post-May-2012 allegiance progression; separate from attribute /15.
    constexpr std::array<unsigned,7> seconds{40,43,47,49,53,56,60};
    constexpr std::array<unsigned,7> percent{100,127,167,193,233,260,300};
    ElementalLordPlan plan;plan.skill=skill;plan.rank=rank;
    plan.duration_ms=seconds[std::min(rank,6u)]*1000;
    plan.healing_percent=percent[std::min(rank,6u)];return plan;
}
inline std::optional<unsigned> elemental_lord_attribute_bonus(const ElementalLordPlan& plan,unsigned attribute){
    if(elemental_lord(plan.skill,plan.rank)!=plan||attribute>51)return {};
    return attribute>=8&&attribute<=11?plan.attribute_bonus:0;
}
struct ElementalLordSpellQuote {
    unsigned base_energy=0,energy_gain=0,displayed_healing_percent=0;
    // This is the published percentage product, NOT a final rounded heal.
    std::uint64_t published_heal_numerator=0;
    unsigned published_heal_denominator=100;
};
// Caller supplies authoritative live effect and admitted spell-use timing.
// No paid cost input exists: spell healing uses catalog base Energy cost.
inline std::optional<ElementalLordSpellQuote> quote_elemental_lord_spell(
        const ElementalLordPlan& plan,unsigned spell,unsigned energy_storage_rank){
    const auto* row=find_skill(spell);
    if(elemental_lord(plan.skill,plan.rank)!=plan||energy_storage_rank>20||
       !row||!row->id||!is_spell(*row))return {};
    const auto cost=row->energy_cost();
    return ElementalLordSpellQuote{cost,energy_storage_rank/10,plan.healing_percent,
        std::uint64_t(cost)*plan.healing_percent,100};
}
}
