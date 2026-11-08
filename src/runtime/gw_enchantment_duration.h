#pragma once
#include "gw_effect_registry.h"
#include "gw_weapon_passives.h"
#include <span>
namespace fw::gw::skills::effects {
class EnchantmentDuration;
inline std::optional<EnchantmentDuration> enchanting_duration(unsigned,Time,Scope,
    const items::weapon_passives::EnchantingEquipment&,std::span<const Entry>,Time);
// Immutable base skill plans remain native. This receipt binds a reviewed
// duration to the caster lifetime and one authoritative equipment snapshot.
class EnchantmentDuration {
 unsigned skill_;Time base_,duration_,at_;Scope caster_;
 std::uint32_t item_;std::uint64_t revision_;
 EnchantmentDuration(unsigned skill,Time base,Time duration,Time at,Scope caster,std::uint32_t item,std::uint64_t revision):
  skill_(skill),base_(base),duration_(duration),at_(at),caster_(caster),item_(item),revision_(revision){}
 friend std::optional<EnchantmentDuration> enchanting_duration(unsigned,Time,Scope,
    const items::weapon_passives::EnchantingEquipment&,std::span<const Entry>,Time);
public:
 std::optional<Time> for_enchantment(unsigned skill,Time base,Scope caster,Time now)const{
  if(skill!=skill_||base!=base_||caster!=caster_||now!=at_)return {};return duration_;
 }
 std::uint32_t item()const{return item_;}
 std::uint64_t revision()const{return revision_;}
};
inline std::optional<EnchantmentDuration> enchanting_duration(unsigned skill,Time base,Scope caster,
        const items::weapon_passives::EnchantingEquipment& equipment,std::span<const Entry> active,Time now){
 const auto* row=find_skill(skill);
 if(!row||row->type!=6||!base||!caster.instance||!caster.actor||!caster.generation)return {};
 // Blessed Aura changes ordering and uses a different rounding rule. Do not
 // silently apply only the weapon part while that unsupported stack is active.
 if(row->profession==3)for(const auto& e:active)if(e.skill==256&&e.kind==Category::enchantment&&e.applied<=now&&(!e.expires||now<*e.expires))return {};
 Time duration=base;
 if(equipment.percent()){
  if(base%1000)return {}; // Fractional native base-duration rounding unverified.
  const auto seconds=base/1000;
  const auto bonus=std::max(Time(1),(seconds*equipment.percent()+50)/100);
  if(seconds>UINT64_MAX/1000-bonus)return {};
  duration=(seconds+bonus)*1000;
 }
 if(now>UINT64_MAX-duration)return {};
 return EnchantmentDuration{skill,base,duration,now,caster,equipment.item(),equipment.revision()};
}
}
