#pragma once
#include "gw_armor_bounds.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include "gw_attack_miss.h"
namespace fw::gw::combat::weapon {
struct Strike {
 unsigned level=0,mastery=0,minimum=0,maximum=0;
 bool caster_weapon=false,customized=false,critical=false,weakness=false,blind=false;
 double base_roll=0,miss_roll=0;
 int location_armor=0;
 unsigned post_penetration_armor=0;
 double additional_miss_chance=0;
 bool critical_immune=false;
 double weapon_base_multiplier=1.;
 // Stoneflesh suppresses the damage treatment, not the critical event.
 bool critical_damage_suppressed=false;
 unsigned item_type=0;
 bool critical_energy_suppressed=false;
};
struct Result {bool missed=false;double damage=0;bool critical=false,critical_energy=false;};
// Reviewed community model in local2021 Damage_calculation.html. This is not
// a claim to have recovered the private ArenaNet server formula. Explicit
// rolls come from server RNG, never from client requests. Critical eligibility
// and chosen armor location must be established before calling this resolver.
// Level zero is accepted only for explicitly reviewed NPC content; default callers retain validation.
inline std::optional<Result> resolve(const Strike& s,bool allow_zero_level=false){
 if((s.weapon_base_multiplier!=1.&&s.weapon_base_multiplier!=.75)||(!s.level&&!allow_zero_level)||s.level>255||s.mastery>20||!s.minimum||s.maximum<s.minimum||s.maximum>1000000||
    !valid_effective_armor(s.location_armor)||s.post_penetration_armor>1000||!std::isfinite(s.base_roll)||s.base_roll<0||s.base_roll>=1||
    !std::isfinite(s.miss_roll)||s.miss_roll<0||s.miss_roll>=1)return {};
 const auto misses=attack_misses(s.miss_roll,s.blind,s.additional_miss_chance);if(!misses)return {};
 if(*misses)return Result{true,0};
 const auto threshold=(s.level+4)/2;
 const double strike=s.caster_weapon?3.0*s.level:5.0*std::min(s.mastery,threshold)+2.0*(s.mastery>threshold?s.mastery-threshold:0);
 // Ordinary damage rolls are explicit inputs because retail random sampling
 // distribution remains unverified. Never infer it from range endpoints.
 const bool event=s.critical&&!s.critical_immune;
 const bool critical=event&&!s.critical_damage_suppressed;
 const double base=critical?s.maximum:s.minimum+(s.maximum-s.minimum)*s.base_roll;
 const auto damage=base*s.weapon_base_multiplier*(s.customized?1.2:1.0)*(s.weakness?.34:1.0)*
     std::exp2((strike+(critical?(s.item_type==35?5:20):0)-s.location_armor-s.post_penetration_armor)/40.0);
 if(!std::isfinite(damage)||damage>16000000)return {};
 return Result{false,damage,event,event&&!s.critical_energy_suppressed};
}
// Bonus damage from attack skills is deliberately separate: Weakness reduces
// weapon base damage only. Requirement fallback, height effects, damage
// modifiers, block, dodge, attack timing and critical probability need their
// own proven policies before world integration.
}

