#pragma once
#include "gw_inventory.h"
#include "gw_effective_weapon_equipment.h"
namespace fw::gw::items::weapon_passives {
// 253 is original879920 name-prefix/suffix/inscription lookup, not a numeric
// combat payload. 240 is a display marker in the original name traversal.
inline bool naming_word(std::uint32_t raw){return (raw&0xffff0000u)==0x24080000u||(raw&0xfffc0000u)==0x25300000u;}
// Original879a10 sends22B's low byte to template2466. GWCA's encoded
// string base100 maps that to AA2, independently identified by ToolboxUtils
// as "Enchantments last N% longer". Only captured22B8/zero-upper-argument
// records and the documented10..20 range are reviewed. This decodes the
// suffix; it does not resolve ownership,active equipment or stacking order.
inline std::optional<unsigned> enchanting_percent(std::span<const std::uint32_t> words){
 std::optional<unsigned> result;
 for(auto raw:words)if((raw&0x3ff00000u)==0x22b00000u){
  if(result||(raw&0xffffff00u)!=0x22b80000u||(raw&255u)<10||(raw&255u)>20)return {};
  result=raw&255u;
 }
 return result.value_or(0);
}
class EnchantingEquipment;
inline std::optional<EnchantingEquipment> equipped_enchanting(const inventory::Profile&,std::uint64_t);
class EnchantingEquipment {
 std::uint32_t item_;unsigned percent_,set_;std::uint64_t revision_;
 EnchantingEquipment(std::uint32_t item,unsigned percent,unsigned set,std::uint64_t revision):
  item_(item),percent_(percent),set_(set),revision_(revision){}
 friend std::optional<EnchantingEquipment> equipped_enchanting(const inventory::Profile&,std::uint64_t);
public:
 unsigned percent()const{return percent_;}
 std::uint32_t item()const{return item_;}
 unsigned active_set()const{return set_;}
 std::uint64_t revision()const{return revision_;}
};
inline std::optional<EnchantingEquipment> equipped_enchanting(const inventory::Profile& profile,std::uint64_t revision){
 weapon_sets::Set active;
 try{active=weapon_sets::effective(profile.weapons,inventory::weapon_lookup(profile));}
 catch(const std::invalid_argument&){return {};}
 if(!active.weapon)return EnchantingEquipment{0,0,profile.weapons.active,revision};
 const auto found=profile.items.find(active.weapon);
 if(found==profile.items.end()||found->first!=found->second.definition.id)return {};
 const auto percent=enchanting_percent(found->second.definition.modifiers);if(!percent)return {};
 return EnchantingEquipment{active.weapon,*percent,profile.weapons.active,revision};
}
inline std::optional<unsigned> energy(std::span<const std::uint32_t> words){
 std::optional<unsigned> result;
 for(auto raw:words)if((raw&0xfffc00ffu)==0x62c80000u){if(result)return {};const auto amount=(raw>>8)&0x3ffu;if(amount>50)return {};result=amount;}
 return result.value_or(0);
}
inline unsigned worn_energy(const inventory::Profile& profile){
 const auto active=inventory::effective_weapon_equipment(profile);if(!active)return 0;
 unsigned total=0;
 for(auto id:{active->weapon,active->offhand}){if(!id)continue;
  const auto amount=energy(profile.items.at(id).definition.modifiers);if(amount)total+=*amount;
 }return total;
}
}
