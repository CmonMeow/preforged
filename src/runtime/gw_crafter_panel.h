#pragma once
#include "gw_arthur_crafter_catalog.h"
namespace fw::gw::crafter {
inline bool supported(unsigned map,std::string_view name){return map==148&&(name=="Arthur Ayala"||name=="Arthur Ayala [Weapons]");}
// Original195 owner -> general definitions ->131 list ->201 type3/float1.
// PRIVATE_ARTHUR_WIKI_CATALOG_V1 membership; captured retail order absent.
inline std::optional<inventory::Batch> open_panel(Binding& active,collectors::native::MenuIds& ids,
 const Observation& at,std::string_view name,const inventory::Profile& owned){
 if(!supported(at.map,name)){active={};return {};}
 auto pending_ids=ids;Binding pending;auto recipes=arthur_wiki_catalog();
 if(!fw::gw::crafter::bind(pending,pending_ids,at,name,owned,std::span<const Recipe>(recipes))){active={};return {};}
 inventory::Batch packets;items::Packet owner;items::put(owner,195,2);items::put(owner,at.actor,4);packets.push_back(std::move(owner));
 items::Packet list;items::put(list,131,2);items::put(list,static_cast<unsigned>(pending.recipes.size()),2);
 for(const auto& row:pending.recipes){auto d=row.reward.definition;d.file|=0x80000000u;packets.push_back(items::definition_2022(d));items::put(list,d.id,4);}
 packets.push_back(std::move(list));items::Packet heading;items::put(heading,201,2);items::put(heading,3,1);items::put(heading,0x3f800000,4);packets.push_back(std::move(heading));
 ids=pending_ids;active=std::move(pending);return packets;
}
inline std::optional<Plan> authorize_wire(Binding& active,std::span<const std::uint8_t> bytes,
 const inventory::Profile& owned,const Observation& at){
 try{return authorize(active,collectors::native::decode(bytes),owned,at);}catch(const std::invalid_argument&){active={};return {};}
}
}
