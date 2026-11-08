#pragma once
#include "gw_effect_registry.h"
#include "gw_combat.h"
namespace fw::gw::skills::effects {
struct ConjurePlan {
    unsigned skill=0,rank=0,damage=0;
    combat::Kind element=combat::Kind::fire;
    Time duration=60000;
    auto operator<=>(const ConjurePlan&)const=default;
};
inline std::optional<ConjurePlan> conjure(unsigned skill,unsigned rank){
    if(rank>20)return {};
    unsigned attribute;combat::Kind element;
    switch(skill){
    case 182:attribute=10;element=combat::Kind::fire;break;
    case 207:attribute=11;element=combat::Kind::cold;break;
    case 221:attribute=8;element=combat::Kind::lightning;break;
    default:return {};
    }
    const auto* row=find_skill(skill);
    if(!row||row->type!=6||row->attribute!=attribute||row->scale0!=5||row->scale15!=20||
       row->duration0!=60||row->duration15!=60||row->energy_cost()!=10||row->recharge!=45)return {};
    return ConjurePlan{skill,rank,5+rank,element,60000};
}
struct ConjureBonus {Entry cause;ConjurePlan plan;};
// Each authoritative landed hit calls this independently. Matching must use
// the attack's effective weapon element after conversions,not item category.
// The caller must preserve elemental identity while bypassing armor for this
// separate damage packet. This selector does not apply damage or consume buffs.
inline std::optional<ConjureBonus> conjure_bonus(const Registry& registry,Scope owner,
        std::optional<combat::Kind> weapon_element,bool landed,Time now){
    if(!landed||!registry.matches(owner)||!weapon_element)return {};
    const unsigned skill=*weapon_element==combat::Kind::fire?182:
        *weapon_element==combat::Kind::cold?207:*weapon_element==combat::Kind::lightning?221:0;
    if(!skill)return {};
    const auto entry=registry.newest_skill(owner,skill,now);
    if(!entry||entry->kind!=Category::enchantment||!entry->cause||!entry->instance)return {};
    const auto plan=conjure(skill,entry->rank);
    return plan?std::optional(ConjureBonus{*entry,*plan}):std::nullopt;
}
struct ConjureHit {combat::Actor actor;combat::Hit hit;ConjureBonus bonus;};
// Value proposal only. World must supply a current,validated attacker and
// target lifetime and commit this after the corresponding weapon hit.
inline std::optional<ConjureHit> prepare_conjure_hit(const Registry& registry,Scope owner,
        std::optional<combat::Kind> weapon_element,bool landed,combat::Actor target,
        unsigned level,bool hostile,Time now){
    const auto bonus=conjure_bonus(registry,owner,weapon_element,landed,now);
    if(!bonus)return {};
    const auto hit=combat::apply(target,{bonus->plan.element,double(bonus->plan.damage),level,bonus->plan.skill,0,true},hostile);
    if(hit.reason!=combat::Reason::accepted)return {};
    return ConjureHit{target,hit,*bonus};
}
}
