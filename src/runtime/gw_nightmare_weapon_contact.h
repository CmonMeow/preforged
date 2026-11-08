#pragma once
// Detached795 payload only. No normal cast binding or World reduction authority.
#include "gw_weapon_spell_charges.h"
#include "gw_life_stealing.h"
#include <array>
namespace fw::gw::skills::effects {
// Arithmetic only: the caller must establish which damage phase supplies this
// scalar. Neither the capture nor this helper authorizes pre/post-armor placement.
inline std::optional<double> nightmare_subtract_at_selected_phase(const ChargedWeaponSpellPlan&plan,double damage){
 if(plan.skill!=795||!valid_charged_weapon_spell(plan)||!std::isfinite(damage)||damage<0||damage>1000000)return {};
 return std::max(0.,damage-plan.amount);
}
struct NightmareContactProposal {
 ChargeProposal charge;
 std::optional<LifeStealResult> transfer;
};
// Archived795 notes count each hit and exclude misses. Failed outcomes have no
// payload. The transfer uses the existing life-steal authority, before weapon
// damage; a future World adapter must preflight and commit the whole contact.
// This snapshot is transaction-local, never a replacement World Health store.
inline std::optional<NightmareContactProposal> prepare_nightmare_contact(const ChargedSpellSnapshot&snapshot,
 const OwnedChargedWeaponSpell&expected,const combat::weapon::ResolvedAttack&event,double life_steal_reduction=0){
 if(expected.plan.skill!=795||!valid_charged_weapon_spell(expected.plan)||!std::isfinite(life_steal_reduction)||life_steal_reduction<0||life_steal_reduction>1000000)return {};
 const bool hit=event.outcome==combat::weapon::AttackOutcome::hit;
 std::optional<LifeStealResult> transfer;std::array<ChargedHealthMutation,2>changes;std::size_t count=0;
 if(hit){
  const auto source=snapshot.health.find(event.attacker),target=snapshot.health.find(event.target);
  if(source==snapshot.health.end()||target==snapshot.health.end())return {};
  const auto health=[](const ChargedHealthFact&fact)->std::optional<combat::Health>{
   if(!fact.valid()||fact.maximum>UINT32_MAX||std::floor(fact.maximum)!=fact.maximum)return {};
   return combat::Health{fact.current,static_cast<unsigned>(fact.maximum)};
  };
  const auto a=health(source->second),b=health(target->second);if(!a||!b)return {};
  transfer=steal_life_amount(expected.plan.amount,*a,*b,life_steal_reduction);if(!transfer)return {};
  const auto append=[&](Scope scope,const ChargedHealthFact&before,double current){
   if(current!=before.current)changes[count++]={scope,before,{current,before.maximum,event.at,before.revision+1}};
  };
  append(event.attacker,source->second,transfer->source.current);append(event.target,target->second,transfer->target.current);
 }
 auto charge=ChargeProposal::prepare(snapshot,expected,event,hit?ChargeConsumption::consume:ChargeConsumption::ignore,std::span(changes.data(),count));
 if(!charge)return {};return NightmareContactProposal{std::move(*charge),transfer};
}
}
