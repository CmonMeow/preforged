#pragma once
#include <optional>
#include "gw_inventory.h"
#include "gw_character_name.h"
#include "gw_attributes.h"
#include <functional>
#include <string>
namespace fw::gw::equipment::proposal {
inline std::optional<unsigned> native_slot(unsigned type){switch(type){
 case 2:case 5:case 15:case 22:case 26:case 27:case 32:case 35:case 36:return 0;
 case 12:case 24:return 1;case 7:return 2;case 19:return 3;case 16:return 4;
 case 4:return 5;case 13:return 6;case 44:return 7;case 45:return 8;default:return {};
}}
struct Reviewed {unsigned slot=0,profession=0;std::optional<unsigned> required_attribute;unsigned required_rank=0;std::string customized_character;bool two_handed=false;};
struct Policy {bool allowed=false;unsigned primary=0;std::string character;attributes::Ranks effective{};std::function<std::optional<Reviewed>(const items::Definition&)> reviewed;};
enum class Reason {accepted,mode,missing,metadata,profession,customization,requirement,quantity,bag,slot,occupied,two_handed};
inline const char* reason_text(Reason reason){switch(reason){
 case Reason::accepted:return "accepted";case Reason::mode:return "mode";case Reason::missing:return "missing";
 case Reason::metadata:return "equipment_metadata_unavailable";case Reason::profession:return "equipment_profession";
 case Reason::customization:return "equipment_customization";case Reason::requirement:return "equipment_requirement";
 case Reason::quantity:return "quantity";case Reason::bag:return "bag";case Reason::slot:return "slot";
 case Reason::occupied:return "occupied";case Reason::two_handed:return "two_handed_unverified";
 }return "invalid";}
struct Result {Reason status;inventory::Profile profile;inventory::Batch packets;bool accepted()const{return status==Reason::accepted;}};
inline Result propose(const inventory::Profile& current,const inventory::Request& request,const Policy& policy){
 inventory::validate(current);auto deny=[&](Reason reason){return Result{reason,current,{}};};
 if(!policy.allowed)return deny(Reason::mode);
 if(request.change!=inventory::Change::equip&&request.change!=inventory::Change::unequip)return deny(Reason::mode);
 const auto equipped=std::find_if(current.bags.begin(),current.bags.end(),[](const auto& pair){return pair.second.type==2&&pair.second.index==21&&pair.second.capacity==9;});
 if(equipped==current.bags.end())return deny(Reason::bag);
 auto source=current.items.end();
 if(request.change==inventory::Change::unequip){if(!request.equipment_slot||*request.equipment_slot>=9)return deny(Reason::slot);const auto id=inventory::owned_request_item(current,request);if(id)source=current.items.find(*id);}
 else source=current.items.find(request.item);
 if(source==current.items.end())return deny(Reason::missing);
 if(source->second.definition.quantity!=1)return deny(Reason::quantity);
 auto next=current;inventory::Batch packets;
 const auto accepts=[&](const inventory::Bag& bag,const inventory::Item& item){
  if(bag.type!=1||bag.index!=4)return true;
  try{return inventory::accepts_item(bag,item,policy.character.empty()?std::u16string{}:character_name_to_utf16(policy.character,31));}
  catch(const std::invalid_argument&){return false;}
 };
 if(request.change==inventory::Change::unequip){
  if(!current.bags.contains(request.bag)||current.bags.at(request.bag).type!=1)return deny(Reason::bag);
  if(request.slot>=current.bags.at(request.bag).capacity)return deny(Reason::slot);
  if(!accepts(current.bags.at(request.bag),source->second))return deny(Reason::customization);
  if(std::any_of(current.items.begin(),current.items.end(),[&](const auto& pair){return pair.second.bag==request.bag&&pair.second.slot==request.slot;}))return deny(Reason::occupied);
  auto& moved=next.items.at(source->first);moved.bag=request.bag;moved.slot=request.slot;packets.push_back(items::location_2022(current.inventory,source->first,moved.bag,moved.slot,true));
 }else{
  if(!policy.reviewed)return deny(Reason::metadata);const auto reviewed=policy.reviewed(source->second.definition);const auto slot=native_slot(source->second.definition.type);
  if(!reviewed||!slot||reviewed->slot!=*slot||reviewed->slot>=9||policy.primary<1||policy.primary>10)return deny(Reason::metadata);
  if(reviewed->profession&&reviewed->profession!=policy.primary)return deny(Reason::profession);
  if(!reviewed->customized_character.empty()&&reviewed->customized_character!=policy.character)return deny(Reason::customization);
  // Retail permits gear below its attribute requirement; only base weapon/
  // shield/focus benefits diminish. Stat provider owns that fallback, upgrades
  // remain effective. Requirement is metadata validation, not an equip gate.
  if(reviewed->required_attribute){if(*reviewed->required_attribute>=51||reviewed->required_rank>20)return deny(Reason::metadata);}else if(reviewed->required_rank)return deny(Reason::metadata);
  if(source->second.bag==equipped->first){if(source->second.slot==*slot)return deny(Reason::accepted);return deny(Reason::slot);}
  if(!current.bags.contains(source->second.bag)||current.bags.at(source->second.bag).type!=1)return deny(Reason::bag);
  // Original2022 retains the equipped offhand under a two-handed mainhand;
  // only its effective benefits are masked. Never relocate or destroy it.
  auto old=std::find_if(current.items.begin(),current.items.end(),[&](const auto& pair){return pair.second.bag==equipped->first&&pair.second.slot==*slot;});
  if(old!=current.items.end()&&!accepts(current.bags.at(source->second.bag),old->second))return deny(Reason::customization);
  if(old!=current.items.end())packets.push_back(items::detach_2022(current.inventory,old->first));
  auto& moved=next.items.at(source->first);moved.bag=equipped->first;moved.slot=static_cast<std::uint8_t>(*slot);packets.push_back(items::location_2022(current.inventory,source->first,moved.bag,moved.slot,true));
  if(old!=current.items.end()){auto& displaced=next.items.at(old->first);displaced.bag=source->second.bag;displaced.slot=source->second.slot;packets.push_back(items::location_2022(current.inventory,old->first,displaced.bag,displaced.slot));}
 }
 // Placement callbacks change the client's active hand references. Persist the
 // same membership and publish all sets after placements, preserving inactive
 // references to displaced items that remain owned in carrying bags.
 if(source->second.slot<2&&request.change==inventory::Change::unequip)
  next.weapons=weapon_sets::unequipped(next.weapons,source->second.slot==0?weapon_sets::Hand::weapon:weapon_sets::Hand::offhand,source->first);
 if(request.change==inventory::Change::equip){const auto slot=native_slot(source->second.definition.type);
  if(slot&&*slot<2)next.weapons=weapon_sets::equipped(next.weapons,*slot==0?weapon_sets::Hand::weapon:weapon_sets::Hand::offhand,source->first);
 }
 inventory::validate(next);
 if(next.weapons!=current.weapons){auto membership=inventory::weapon_packets(next);packets.insert(packets.end(),membership.begin(),membership.end());}
 return {Reason::accepted,std::move(next),std::move(packets)};
}
}
