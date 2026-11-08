#pragma once
#include "gw_armor_attribute_runes.h"
#include "gw_reviewed_item_stats.h"
namespace fw::gw::items::requirements {
enum class Origin {unknown,random_drop,nonrandom};
struct Base {std::optional<unsigned> shield_armor,focus_energy;Origin origin=Origin::unknown;};
struct Resolved {bool requirement_met=true;std::optional<unsigned> shield_armor,focus_energy;};
// Trusted base stats and provenance are explicit: raw names/model IDs cannot
// establish drop origin. Upgrade bonuses are outside the requirement penalty.
inline std::optional<Resolved> resolve(std::span<const std::uint32_t> words,const Base& base,const std::array<std::uint8_t,51>& effective){
 unsigned count=0;for(auto w:words)if((w&0x3ff00000u)==0x27900000u)++count;
 if(count>1||(base.shield_armor&&base.focus_energy))return {};
 const auto req=equipment::attribute_runes::requirement(words);
 if(req&&(req->attribute>=effective.size()||attributes::catalog[req->attribute].profession==0||req->rank>20))return {};
 Resolved out;out.requirement_met=!req||effective[req->attribute]>=req->rank;
 out.shield_armor=base.shield_armor;out.focus_energy=base.focus_energy;
 if(base.shield_armor&&(*base.shield_armor<1||*base.shield_armor>16))return {};
 if(base.focus_energy&&(*base.focus_energy<1||*base.focus_energy>12))return {};
 if(out.requirement_met)return out;
 if(base.shield_armor){
  if(*base.shield_armor==16||base.origin==Origin::random_drop)out.shield_armor=8;
  else if(base.origin==Origin::nonrandom)out.shield_armor=5;else out.shield_armor.reset();
 }
 if(base.focus_energy){
  if(base.origin==Origin::random_drop&&*base.focus_energy>=7)out.focus_energy=6;
  else if(base.origin==Origin::nonrandom)out.focus_energy=3;else out.focus_energy.reset();
 }
 return out;
}
// Exact raw damage remains observable even without proven unmet-requirement
// damage math (wiki gives only approximately two-thirds). Do not round a guess.
inline std::optional<stats::WeaponDamage> usable_weapon_damage(std::span<const std::uint32_t> words,const std::array<std::uint8_t,51>& effective){
 const auto state=resolve(words,{},effective);if(!state||!state->requirement_met)return {};
 return stats::reviewed_weapon_damage(modifiers::decode_wire(words));
}
}
