#pragma once
#include "gw_effect_registry.h"
#include "gw_conditions.h"
#include "gw_ending_hex_context.h"
#include "gw_skill_effect_plan.h"
#include <compare>
#include <cmath>
#include <span>
namespace fw::gw::skills::effects {
enum class WeaponSpellKind {resilient,warding,wailing,splinter};
struct WeaponSpellPlan {
 unsigned skill=0,rank=0,primary=0,spawning=0;WeaponSpellKind kind{};
 std::uint64_t duration=0;unsigned pips=0,armor=0,block=0;bool ends_on_attack=false;unsigned charges=0,amount=0;
 auto operator<=>(const WeaponSpellPlan&)const=default;
};
// Archived SpawningPower: nearest whole second,exact half rounds down.
// Two archived breakpoint-table cells disagree;this is explicit textual policy,
// not a recovered retail server formula. Integer arithmetic avoids float ties.
inline std::optional<std::uint64_t> weapon_spell_duration(unsigned seconds,unsigned spawning){
 if(!seconds||seconds>3600||spawning>20)return {};
 return std::uint64_t((seconds*(100+4*spawning)+49)/100)*1000;
}
// Canonical finite-charge metadata, shared with the atomic contact ledger.
// Only reviewed792/2868 are admitted below;795 has no production consumer.
struct ChargedWeaponSpellPlan {
 unsigned skill=0,rank=0,primary=0,spawning=0,charges=0,amount=0;
 std::uint64_t duration=0;
 bool operator==(const ChargedWeaponSpellPlan&)const=default;
};
inline std::optional<ChargedWeaponSpellPlan> charged_weapon_spell(unsigned skill,unsigned rank,unsigned primary,unsigned spawning){
 if((skill!=792&&skill!=2868&&skill!=795)||rank>20||!primary||primary>10||spawning>20||(primary!=8&&spawning))return {};
 const auto*r=find_skill(skill);const bool splinter=skill==792||skill==2868,pvp=skill==2868;
 if(!r||r->profession!=8||r->type!=25||r->attribute!=34||r->special!=(pvp?0x400000u:0u)||r->energy_cost()!=5||r->activation!=1.f||r->aftercast!=.75f||
    r->duration0!=(splinter?20u:12u)||r->duration15!=r->duration0||r->recharge!=(splinter?(pvp?8u:5u):10u)||
    r->scale0!=(splinter?5u:10u)||r->scale15!=(pvp?30u:50u)||r->bonus0!=(splinter?1u:3u)||r->bonus15!=(splinter?5u:3u)||
    r->caster_body_resource!=2077||r->target_body_resource!=(splinter?2077u:1158u))return {};
 const auto duration=weapon_spell_duration(splinter?20:12,spawning);if(!duration)return {};
 return ChargedWeaponSpellPlan{skill,rank,primary,spawning,splinter?static_cast<unsigned>(std::lround(1.+4.*rank/15.)):3u,
  static_cast<unsigned>(std::lround((splinter?5.:10.)+(splinter?(pvp?25.:45.):40.)*rank/15.)),*duration};
}
inline bool valid_charged_weapon_spell(const ChargedWeaponSpellPlan&p){const auto expected=charged_weapon_spell(p.skill,p.rank,p.primary,p.spawning);return expected&&*expected==p;}
inline std::optional<WeaponSpellPlan> weapon_spell(unsigned skill,unsigned rank,unsigned primary,unsigned spawning){
 if(rank>20||!primary||primary>10||spawning>20||(primary!=8&&spawning))return {};
 if(skill!=787&&skill!=792&&skill!=2868&&skill!=793&&skill!=794&&skill!=2893)return {};
 if(skill==792||skill==2868){
  const auto charged=charged_weapon_spell(skill,rank,primary,spawning);if(!charged)return {};
  return WeaponSpellPlan{skill,rank,primary,spawning,WeaponSpellKind::splinter,charged->duration,0,0,0,false,charged->charges,charged->amount};
 }

 const auto*r=find_skill(skill);const bool resilient=skill==787,wailing=skill==794,pvp=skill==2893;
 const unsigned last=resilient?12:wailing?9:8;
 if(!r||r->profession!=8||r->type!=25||r->attribute!=(wailing?34u:33u)||r->special!=(pvp?0x400000u:0u)||
    r->energy_cost()!=(wailing?5u:10u)||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=(resilient?6u:wailing?25u:pvp?8u:5u)||
    r->duration0!=3||r->duration15!=last||r->scale0!=(resilient?1u:wailing?0u:50u)||r->scale15!=(resilient?6u:wailing?0u:50u)||
    r->bonus0!=(resilient?24u:wailing?0u:2u)||r->bonus15!=(resilient?24u:wailing?0u:4u)||
    r->caster_body_resource!=(resilient?1282u:2077u)||r->target_body_resource!=(resilient?1167u:wailing?1156u:1154u))return {};
 const auto duration=weapon_spell_duration(interpolate(3,last,rank),spawning);if(!duration)return {};
 return WeaponSpellPlan{skill,rank,primary,spawning,resilient?WeaponSpellKind::resilient:wailing?WeaponSpellKind::wailing:WeaponSpellKind::warding,
   *duration,resilient?interpolate(1,6,rank):wailing?0u:interpolate(2,4,rank),resilient?24u:0u,wailing||resilient?0u:50u,pvp};
}
inline bool valid_weapon_spell(const WeaponSpellPlan&p){const auto expected=weapon_spell(p.skill,p.rank,p.primary,p.spawning);return expected&&*expected==p;}
struct OwnedWeaponSpell {
 Scope source,target;Entry entry;WeaponSpellPlan plan;
 bool active(const Registry&registry,Scope recipient,std::uint64_t now)const{
  if(recipient!=target||source.instance!=target.instance||!source.actor||!source.generation||!registry.matches(target)||!valid_weapon_spell(plan)||
     entry.cause!=source.actor||entry.skill!=plan.skill||entry.rank!=plan.rank||entry.kind!=Category::weapon_spell||!entry.instance||
     !entry.expires||entry.applied>now||now>=*entry.expires||*entry.expires-entry.applied!=plan.duration)return false;
  const auto actual=registry.lookup(target,entry.instance);
  return actual&&actual->instance==entry.instance&&actual->cause==entry.cause&&actual->skill==entry.skill&&actual->rank==entry.rank&&
    actual->kind==entry.kind&&actual->applied==entry.applied&&actual->expires==entry.expires;
 }
};
inline unsigned weapon_spell_pips(const OwnedWeaponSpell&spell,const Registry&registry,Scope target,const combat::conditions::State&conditions,std::uint64_t now){
 if(!spell.active(registry,target,now))return 0;
 if(spell.plan.kind==WeaponSpellKind::resilient&&!conditions.count(now)&&!registry.count(target,Category::hex,now))return 0;
 return spell.plan.pips;
}
inline unsigned weapon_spell_armor(const OwnedWeaponSpell&spell,const Registry&registry,Scope target,const combat::conditions::State&conditions,std::uint64_t now,const EndingHexContext*ending_hex=nullptr){
 return spell.active(registry,target,now)&&spell.plan.kind==WeaponSpellKind::resilient&&
  ((ending_hex&&ending_hex->qualifies(target,now))||conditions.count(now)||registry.count(target,Category::hex,now))?spell.plan.armor:0;
}
inline std::optional<Entry> warding_block_entry(const OwnedWeaponSpell&spell,const Registry&registry,Scope recipient,std::uint64_t now){
 return spell.active(registry,recipient,now)&&spell.plan.kind==WeaponSpellKind::warding?std::optional(spell.entry):std::nullopt;
}

}
