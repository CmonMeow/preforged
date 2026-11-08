#pragma once
#include "gw_inventory.h"
#include "gw_item_base_armor.h"
#include "gw_armor_attribute_runes.h"
#include "gw_holy_damage.h"
namespace fw::gw::equipment {
// Ordinary authored human players have neither NPC holy sensitivity nor
// summoned status. Equipment vulnerability must be established separately.
// Unknown captured armor/upgrades remain unknown, never a zero bonus.
inline std::optional<unsigned> reviewed_player_holy_bonus(const inventory::Profile& profile,unsigned primary){
 if(primary<1||primary>10)return {};
 unsigned bonus=0;std::array<bool,5> seen{};constexpr unsigned types[]{0,0,7,19,16,4,13};
 for(const auto&[id,item]:profile.items){const auto bag=profile.bags.find(item.bag);
  if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot<2||item.slot>6)continue;
  if(seen[item.slot-2]||!item.reviewed_armor||item.profession!=primary||item.definition.quantity!=1||item.definition.type!=types[item.slot])return {};
  seen[item.slot-2]=true;const auto definition=inventory::presented_definition(item);
  if(!items::base_armor::decode(definition.modifiers)||!attribute_runes::decode(definition.modifiers,primary).valid)return {};
  for(auto word:definition.modifiers){const auto property=(word>>20)&0x3ffu;
   if(property==0x23c||property==0x22c||property==0x22e||property==0x21e)continue;
   switch(word){
    case 0x27e802ea:case 0x27e902ea:case 0x27ea02ea:
    case 0x27e802c2:case 0x27e902c2:case 0x27ea02c2:
    case 0x25300423:case 0x25300425:case 0x253003ca:case 0x253003cc:case 0xa53003f4:case 0xa53003ce:break;
    case 0x253003d8:bonus+=item.slot==2?6:item.slot==3?4:2;break;
    default:return {};
   }
  }
 }
 return bonus;
}
}
