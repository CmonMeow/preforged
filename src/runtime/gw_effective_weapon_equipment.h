#pragma once
#include "gw_inventory.h"
namespace fw::gw::inventory {
// A stored weapon-set reference alone is not a currently wielded item.
// Native twohand masking preserves the physical offhand and stored reference.
inline std::optional<weapon_sets::Set> effective_weapon_equipment(const Profile& profile){
 try{
  const auto effective=weapon_sets::effective(profile.weapons,weapon_lookup(profile));
  for(const auto& [id,slot]:{std::pair{effective.weapon,0u},std::pair{effective.offhand,1u}}){
   if(!id)continue;const auto item=profile.items.find(id);if(item==profile.items.end())return {};
   const auto bag=profile.bags.find(item->second.bag);
   if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||
      item->second.slot!=slot||item->second.definition.quantity!=1)return {};
  }
  return effective;
 }catch(const std::invalid_argument&){return {};}
}
}
