#pragma once
#include "gw_weapon_intent.h"
namespace fw::gw::combat::weapon {
struct OrdinaryWeaponTimingPolicy {
 enum class Basis {starter_recurve_policy,caster_policy,hammer,scythe,legacy_melee_fallback};
 Basis basis;Timing base;bool hammer=false;
};
// Repeat period and contact windup are independent facts. Local Scythe revision
//2658200 / Weapon table give1.50s repeats. Existing500ms release windup stays
// development policy; no new contact fraction or Scythe collateral is inferred.
inline OrdinaryWeaponTimingPolicy ordinary_weapon_timing_policy(const Owned& weapon){
 if(weapon.item_type==5)return {OrdinaryWeaponTimingPolicy::Basis::starter_recurve_policy,{500,2475,1200},false};
 if(weapon.caster_weapon)return {OrdinaryWeaponTimingPolicy::Basis::caster_policy,{500,1750,1200},false};
 if(weapon.item_type==15)return {OrdinaryWeaponTimingPolicy::Basis::hammer,{500,1750,128},true};
 if(weapon.item_type==35)return {OrdinaryWeaponTimingPolicy::Basis::scythe,{500,1500,128},false};
 // Preserve preexisting behavior for all remaining admitted weapon profiles.
 // This compatibility branch is NOT newly reviewed weapon cadence metadata.
 return {OrdinaryWeaponTimingPolicy::Basis::legacy_melee_fallback,{500,1330,128},false};
}
}
