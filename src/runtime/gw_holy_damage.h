#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_castigation_signet.h"
#include <algorithm>
#include <compare>
#include <vector>
namespace fw::gw::skills::effects {
// Hidden creature traits are reviewed independently of appearance or species.
// An absent value is unknown, not false. These describe NPCs; player armor
// insignia and transformations require a separate authoritative equipment path.
struct HolyTraits {
    std::optional<bool> light_sensitive, summoned;
};
struct HolyPlan {
    unsigned skill=0,rank=0,damage=0;
    float range=1248;
    unsigned knocked_down_bonus=0;
    unsigned energy_if_attacking=0;
    unsigned enchantments=0;
    auto operator<=>(const HolyPlan&)const=default;
};
inline std::optional<HolyPlan> holy_damage(unsigned skill,unsigned rank=0,unsigned enchantments=0){
    if(skill==1689){
        const auto* row=find_skill(skill);
        if(rank>20||enchantments>20||!row||row->profession!=3||row->type!=7||row->attribute!=14||
           row->energy_cost()!=0||row->activation!=2.f||row->aftercast!=.75f||row->recharge!=20||
           row->scale0!=5||row->scale15!=35||row->bonus0!=100||row->bonus15!=100||
           row->caster_body_resource!=2077||row->target_body_resource!=1658)return {};
        return HolyPlan{1689,rank,std::min(100u,(5u+2u*rank)*enchantments),1248.f,0,0,enchantments};
    }
    if(skill==2006){const auto plan=castigation_signet(rank);if(!plan)return {};return HolyPlan{2006,rank,plan->damage,plan->range,0,plan->energy};}
    if((skill!=252&&skill!=296&&skill!=312&&skill!=262&&skill!=294&&skill!=2887)||rank>20||enchantments)return {};
    if(skill==294||skill==2887){const auto* row=find_skill(skill);const bool pvp=skill==2887;
        if(!row||row->profession!=3||row->type!=7||row->attribute!=14||row->special!=(pvp?4194316u:4u)||row->energy_cost()!=0||row->health_sacrifice||row->adrenaline||
           row->activation!=1.f||row->aftercast!=.75f||row->recharge!=20||row->pvp_skill!=(pvp?294u:2887u)||
           row->scale0!=(pvp?5u:15u)||row->scale15!=(pvp?50u:75u)||row->bonus0||row->bonus15||row->caster_body_resource!=519||row->target_body_resource!=2077)return {};
        return HolyPlan{skill,rank,pvp?5+3*rank:15+4*rank,pvp?624.f:1248.f};}
    if(skill==262){const auto* row=find_skill(skill);
        if(!row||row->type!=6||row->attribute!=14||row->special!=4||row->energy_cost()!=15||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=45||
           row->scale0!=5||row->scale15!=50||row->duration0!=8||row->duration15!=20||row->target_body_resource!=475)return {};
        return HolyPlan{262,rank,5+3*rank,0};}
    if(skill==312){const auto* row=find_skill(skill);
        if(!row||row->type!=10||row->attribute!=14||row->energy_cost()!=5||row->activation!=.75f||row->recharge!=8||
           row->scale0!=10||row->scale15!=55||row->bonus0!=10||row->bonus15!=55||row->target_body_resource!=556)return {};
        const auto damage=interpolate(10,55,rank);return HolyPlan{312,rank,damage,144,damage};}
    if(skill==296)return HolyPlan{skill,rank,26+2*rank};
    return HolyPlan{skill,rank,interpolate(20,56,rank)};
}
inline std::optional<combat::Damage> holy_strike(const HolyPlan& plan,const HolyTraits& traits,unsigned level){
    const auto expected=holy_damage(plan.skill,plan.rank,plan.enchantments);
    if(!expected||*expected!=plan||!traits.light_sensitive||(plan.skill==252&&!traits.summoned))return {};
    const unsigned multiplier=(*traits.light_sensitive?2u:1u)*(plan.skill==252&&*traits.summoned?2u:1u);
    combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage*multiplier),level,plan.skill};
    return combat::valid(damage)?std::optional(damage):std::nullopt;
}
inline std::optional<std::vector<combat::Damage>> holy_strikes(const HolyPlan& plan,const HolyTraits& traits,unsigned level,bool knocked_down){
    const auto first=holy_strike(plan,traits,level);if(!first)return {};
    std::vector<combat::Damage> hits{*first};
    if(knocked_down&&plan.knocked_down_bonus){auto bonus=*first;bonus.amount=double(plan.knocked_down_bonus)*(*traits.light_sensitive?2u:1u);hits.push_back(bonus);}
    return hits;
}
}
