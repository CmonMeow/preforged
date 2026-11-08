#pragma once
#include "gw_condition_item_modifiers.h"
#include "gw_effective_weapon_equipment.h"
namespace fw::gw::equipment {
class ConditionEquipment;
inline std::optional<ConditionEquipment> condition_equipment(const inventory::Profile&,std::uint64_t);
class ConditionEquipment {
 items::modifiers::ConditionBonuses bonuses_;
 std::uint32_t weapon_=0,offhand_=0;std::array<std::uint32_t,5> armor_{};
 unsigned active_=0;std::uint64_t revision_=0;
 friend std::optional<ConditionEquipment> condition_equipment(const inventory::Profile&,std::uint64_t);
public:
 const auto& bonuses()const{return bonuses_;}
 auto weapon()const{return weapon_;}auto offhand()const{return offhand_;}
 const auto& armor()const{return armor_;}auto active_set()const{return active_;}
 auto revision()const{return revision_;}
};
// Carrying an item or storing a set reference is not an active equipment grant.
// No cached bonus survives switching, two-hand masking or bundle suppression.
inline std::optional<ConditionEquipment> condition_equipment(const inventory::Profile& profile,std::uint64_t revision){
 const auto active=inventory::effective_weapon_equipment(profile);if(!active)return {};
 ConditionEquipment result;result.weapon_=active->weapon;result.offhand_=active->offhand;
 result.active_=profile.weapons.active;result.revision_=revision;
 const auto any=[](const auto& flags){return std::any_of(flags.begin(),flags.end(),[](bool b){return b;});};
 for(const auto [id,offhand]:{std::pair{active->weapon,false},std::pair{active->offhand,true}}){
  if(!id)continue;const auto& item=profile.items.at(id);
  if(item.definition.id!=id)return {};
  const auto mods=items::modifiers::condition_bonuses(item.definition.modifiers);if(!mods)return {};
  if(any(mods->rune)||(offhand?any(mods->extension):any(mods->inscription)))return {};
  if(offhand)result.bonuses_.inscription=mods->inscription;else result.bonuses_.extension=mods->extension;
 }
 constexpr unsigned types[]{7,19,16,4,13};
 for(const auto& [id,item]:profile.items){
  const auto bag=profile.bags.find(item.bag);
  if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot<2||item.slot>6)continue;
  const auto slot=item.slot-2;
  if(result.armor_[slot]||item.definition.id!=id||item.definition.quantity!=1||item.definition.type!=types[slot])return {};
  const auto mods=items::modifiers::condition_bonuses(item.definition.modifiers);
  if(!mods||any(mods->extension)||any(mods->inscription))return {};
  result.armor_[slot]=id;
  for(unsigned i=0;i<10;++i)result.bonuses_.rune[i]=result.bonuses_.rune[i]||mods->rune[i];
 }
 return result;
}
}
