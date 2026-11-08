#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_inventory.h"
#include <span>
#include <tuple>
#include <algorithm>

namespace fw::gw::skills::effects {
struct ShieldStancePlan {
 unsigned rank=0;Time duration=1000;double block=.75;
 bool operator==(const ShieldStancePlan&)const=default;
};
inline std::optional<ShieldStancePlan> shield_stance(unsigned rank){
 const auto* r=find_skill(378);
 if(rank>20||!r||r->type!=3||r->attribute!=21||r->energy_cost()!=5||r->recharge!=15||
    r->activation!=0||r->aftercast!=0||r->duration0!=1||r->duration15!=6||
    r->scale0!=75||r->scale15!=75||r->bonus0!=15||r->bonus15!=15||
    r->caster_body_resource!=637||r->target_body_resource!=2077)return {};
 return ShieldStancePlan{rank,1000ull*interpolate(1,6,rank)};
}
struct ShieldEquipment {
 std::uint64_t revision=0;std::uint32_t shield=0;
 bool operator==(const ShieldEquipment&)const=default;
};
// The caller owns the profile/revision snapshot. A stored shield suppressed by
// a two-handed weapon is not wielded. Unknown/invalid ownership is not "no shield".
inline std::optional<ShieldEquipment> shield_equipment(const inventory::Profile& profile,std::uint64_t revision){
 if(!revision)return {};
 try{
  const auto lookup=inventory::weapon_lookup(profile);
  const auto effective=weapon_sets::effective(profile.weapons,lookup);
  if(!effective.offhand)return ShieldEquipment{revision,0};
  const auto item=lookup(effective.offhand);
  if(!item)return {};
  return ShieldEquipment{revision,item->type==24?effective.offhand:0};
 }catch(const std::invalid_argument&){return {};}
}
struct ShieldStanceProtection {
 unsigned instance=0,reduction=0;double block=0;
 bool operator==(const ShieldStanceProtection&)const=default;
};
// Evaluate current equipment and current effective Strength at contact, not
// appearance or cast-time gear. Unknown facts reject rather than grant defense.
// Registry lifetime/replacement remains the owner's responsibility.
inline std::optional<ShieldStanceProtection> shield_stance_protection(
 std::span<const Entry> entries,Time now,std::optional<ShieldEquipment> equipment,std::optional<unsigned> strength){
 const Entry* selected=nullptr;
 for(const auto& e:entries){
  if(e.skill!=378||e.kind!=Category::stance||!e.instance||!e.cause||e.applied>now||!e.expires||now>=*e.expires)continue;
  if(!shield_stance(e.rank))return {};
  if(!selected||std::tie(e.applied,e.instance)>std::tie(selected->applied,selected->instance))selected=&e;
 }
 if(!selected)return ShieldStanceProtection{};
 if(!equipment||!equipment->revision||!strength||*strength>20)return {};
 if(!equipment->shield)return ShieldStanceProtection{};
 return ShieldStanceProtection{selected->instance,std::min(15u,2u * *strength),.75};
}
}
