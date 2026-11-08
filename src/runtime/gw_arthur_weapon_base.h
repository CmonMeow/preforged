#pragma once
#include "gw_arthur_crafter_catalog.h"
#include "gw_requirement_stats.h"
namespace fw::gw::items::arthur_weapon {
struct Base {
 stats::WeaponDamage damage;std::optional<unsigned> requirement_attribute;unsigned requirement_rank=0;
 unsigned health_bonus=0,charr_damage_bonus=0;bool rinblade_armor_bug=false;
};
// PRIVATE_ARTHUR_WIKI_CATALOG_V1 exact complete-definition family only.
// No global A488 decoder/unknown-modifier fallback. Metadata/physical ownership
// remain the existing owned_weapon caller's responsibility. Conditional state
// is reported explicitly; this helper does not apply Health or Charr benefits.
inline std::optional<Base> reviewed(const Definition& definition){
 static const auto catalog=crafter::arthur_wiki_catalog();
 for(const auto& recipe:catalog){
  const auto& expected=recipe.reward.definition;
  if(expected.type==12)continue;
  // Immutable catalog cache; all definition fields except allocated ownedID
  // compare directly, without constructing serialized packets in the hotpath.
  if(expected.file!=definition.file||expected.type!=definition.type||expected.unknown1!=definition.unknown1||
     expected.dye!=definition.dye||expected.materials!=definition.materials||expected.unknown2!=definition.unknown2||
     expected.interaction!=definition.interaction||expected.price!=definition.price||expected.model!=definition.model||
     expected.quantity!=definition.quantity||expected.encoded_name!=definition.encoded_name||expected.modifiers!=definition.modifiers)continue;
  switch(definition.type){
   case 2:return Base{{5,10,2},{},0,5,0,false};
   case 5:return Base{{7,10,1},{},0,0,1,false};
   case 15:return Base{{13,20,0},19,3,0,1,false};
   case 27:return Base{{10,14,2},20,3,0,0,true};
  }
 }return {};
}
inline std::optional<stats::WeaponDamage> usable_base(const Definition& definition,const std::array<std::uint8_t,51>& effective){
 const auto base=reviewed(definition);if(!base)return {};
 if(base->requirement_attribute&&effective[*base->requirement_attribute]<base->requirement_rank)return {};
 return base->damage;
}
}
