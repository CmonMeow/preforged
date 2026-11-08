#pragma once
#include "gw_effect_registry.h"
#include "gw_attack_outcomes.h"
#include "gw_skillbar.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct IllusionaryWeaponryPlan {
 unsigned skill=33,rank=0,damage=8,armor_per_skill=5;
 Time duration=30000;
 auto operator<=>(const IllusionaryWeaponryPlan&)const=default;
};
inline std::optional<IllusionaryWeaponryPlan> illusionary_weaponry(unsigned skill,unsigned rank){
 const auto* row=find_skill(skill);
 if(rank>20||(skill!=33&&skill!=3181)||!row||row->type!=6||row->attribute!=1||
    row->scale0!=8||row->scale15!=40||row->duration0!=30||row->duration15!=30||
    row->bonus0!=(skill==33?5u:0u)||row->bonus15!=row->bonus0)return {};
 return IllusionaryWeaponryPlan{skill,rank,static_cast<unsigned>(std::lround(8.+32.*rank/15.)),skill==33?5u:0u,30000};
}
inline std::optional<unsigned> illusionary_weaponry_armor(const IllusionaryWeaponryPlan& plan,const skills::Slots& slots){
 const auto expected=illusionary_weaponry(plan.skill,plan.rank);
 if(!expected||*expected!=plan)return {};
 unsigned count=0;
 for(const auto slot:slots){
  if(!slot.skill)continue;
  const auto* row=find_skill(slot.skill);if(!row)return {};
  if(row->attribute==1)++count;
 }
 return count*plan.armor_per_skill;
}
// Replacement damage belongs to the enchantment. It must not qualify a hit or
// a failed hit, apply weapon bonuses, interrupt via Dazed, or trigger on-hit
// benefits. The owned attack dispatcher remains responsible for melee range,
// legitimate sequence, generation and recipient admission before calling this.
struct IllusionaryWeaponryReplacement {
 IllusionaryWeaponryPlan plan;
 unsigned effect=0;
 combat::weapon::AttackOutcome outcome=combat::weapon::AttackOutcome::replaced;
};
inline std::optional<IllusionaryWeaponryReplacement> illusionary_weaponry_replacement(
 const Registry& registry,Scope owner,Time now,bool melee,bool valid_sequence){
 if(!melee||!valid_sequence||!owner.instance||!owner.actor||!owner.generation)return {};
 const auto entry=registry.newest_skill(owner,33,now,3181);
 if(!entry||entry->kind!=Category::enchantment||entry->cause!=owner.actor||!entry->instance||
    entry->applied>now||!entry->expires||now>=*entry->expires)return {};
 const auto plan=illusionary_weaponry(entry->skill,entry->rank);
 return plan?std::optional<IllusionaryWeaponryReplacement>{{*plan,entry->instance}}:std::nullopt;
}
}
