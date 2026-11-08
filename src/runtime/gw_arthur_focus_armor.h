#pragma once
#include "gw_arthur_focus_catalog.h"
#include "gw_effective_weapon_equipment.h"
namespace fw::gw::items::arthur_focus {
// Intrinsic armor, outside conditional armor-bonus caps. Original879a10 reads
// exact modifiers62c80300/a3c80500 as Energy3/AR5; wiki agrees for all9 rows.
// Full definition identity and explicit issuance metadata are required.
inline bool trusted(const inventory::Item& item){
 if(item.profession!=0||!item.customized||!item.customized->empty()||item.reviewed_armor)return false;
 for(const auto& row:crafter::arthur_focus_candidates()){
  auto expected=row.reward.definition;expected.id=item.definition.id;
  if(definition_2022(expected)==definition_2022(item.definition))return true;
 }return false;
}
inline unsigned armor_bonus(const inventory::Profile& profile){
 const auto active=inventory::effective_weapon_equipment(profile);
 return active&&active->offhand&&trusted(profile.items.at(active->offhand))?5u:0u;
}
}
