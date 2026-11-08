#pragma once
#include "gw_inventory.h"

namespace fw::gw::storage {
// Account-owned storage. Opening/permission is authoritative server state;
// receiving an arbitrary bag ID never grants access to a bank.
inline inventory::Profile baseline(){inventory::Profile p;p.bags={{100,{100,4,7,25,0}},{101,{101,5,5,41,0}}};return p;}
inline void validate(const inventory::Profile& p){inventory::validate(p);for(auto [id,b]:p.bags)if((b.type!=4&&b.type!=5)||b.container)throw std::invalid_argument("invalid account storage bag");}
// Entitlement input is server-owned. Add panes without revoking purchased panes
// or replacing an existing profile. Caller persists with revision checks.
inline inventory::Profile provision_panes(const inventory::Profile& current,unsigned panes){
 storage::validate(current);if(panes<1||panes>14)throw std::invalid_argument("storage pane count");
 auto result=current;
 for(unsigned index=7;index<7+panes;++index){
  if(std::any_of(result.bags.begin(),result.bags.end(),[&](const auto& entry){return entry.second.type==4&&entry.second.index==index;}))continue;
  unsigned id=100;while(id<=UINT16_MAX&&result.bags.contains(static_cast<std::uint16_t>(id)))++id;
  if(id>UINT16_MAX)throw std::invalid_argument("storage bag identity exhausted");
  result.bags.emplace(static_cast<std::uint16_t>(id),inventory::Bag{static_cast<std::uint16_t>(id),4,static_cast<std::uint8_t>(index),25,0});
 }
 storage::validate(result);return result;
}
struct Policy {bool chest_open=false;bool can_deposit=false;bool can_withdraw=false;std::u16string character;};
// Pinned130 type0 publishes UI10000040 with data&1/data&2. OldGWCA
// OpenXunlaiWindow uses data3. Server authorizes actual nearby Xunlai actor.
inline inventory::Packet open_window(std::uint32_t npc){if(!npc)throw std::invalid_argument("authorized Xunlai actor required");inventory::Packet p;items::put(p,130,2);items::put(p,npc,4);items::put(p,0,1);items::put(p,3,4);return p;}
// Session-only, unplaced container records. Storage UI 85421c calls7c3010
// and7c3420 to name each bag; container0 asserts itcliapi985. These IDs are
// presentation references, never owned/transferable items or durable grants.
inline constexpr std::uint32_t container_first=60000,container_last=60004;
inline void validate_presentation_ids(const inventory::Profile& p){for(const auto& [id,item]:p.items)if(id>=container_first&&id<=container_last)throw std::invalid_argument("owned item collides with storage presentation identity");}
inline items::Definition presentation_container(unsigned index,unsigned capacity=0){if(index>4)throw std::invalid_argument("regular container index");items::Definition d;d.id=container_first+index;d.file=1;d.type=3;d.quantity=1;d.interaction=4;
 // Stable own-container capture items-93194218, independently rendered in
 // the pinned2022 client: physical Backpack asset, not a composite index.
 // Session-only presentation identity is unchanged; no owned item is granted.
 if(index==0&&(!capacity||capacity==20)){
  d.file=111926;d.model=32;d.unknown1=1;d.dye=9;d.materials=1510;
  d.interaction=0x20001000;d.price=5;d.encoded_name=u"\x21a8\xd157\xb58f\x166f";
  d.modifiers={0x24481400};return d;
 }
 //File0 asserts in352. Inventory's bag bar renders even an unplaced container.
 //COMPOSITE4 interprets1 as a composite index, never physical DAT file1 (INI).
 //Pinned2022/current DAT row1 header03440001 is a non-item:7b1850 returns0;
 //7af820 then generates its native no-geometry texture without opening files.
 //This session-only placeholder has no captured Backpack icon or owned item.
 d.encoded_name=u"\x108\x107";d.encoded_name+=index==0?u"Backpack":index==1?u"Belt Pouch":index==4?u"Equipment Pack":u"Bag";d.encoded_name+=u'\x1';return d;}
inline inventory::Batch restore_character_placements(const inventory::Profile& character){
 inventory::validate(character);inventory::Batch p;
 for(const auto& [id,item]:character.items)if(!inventory::installed(item))p.push_back(items::location_2022(character.inventory,id,item.bag,item.slot));
 auto sets=inventory::weapon_packets(character);p.insert(p.end(),sets.begin(),sets.end());return p;
}
inline inventory::Batch restore_character(const inventory::Profile& character,bool inventory_exists=true,bool include_placements=true){
 inventory::validate(character);validate_presentation_ids(character);inventory::Batch p;if(!inventory_exists)p.push_back(items::inventory_2022(character.inventory));
 //All container definitions must exist BEFORE318 looks them up at7c4648.
 for(const auto& [id,item]:character.items){auto metadata=inventory::item_packets(item);p.insert(p.end(),metadata.begin(),metadata.end());}
 for(const auto& [id,bag]:character.bags)if(bag.type==1&&!bag.container)p.push_back(items::definition_2022(presentation_container(bag.index,bag.capacity)));
 for(const auto& [id,bag]:character.bags)p.push_back(items::bag_2022(character.inventory,bag.type,bag.index,id,bag.capacity,bag.type==1&&!bag.container?container_first+bag.index:bag.container));
 if(include_placements){auto placements=restore_character_placements(character);p.insert(p.end(),placements.begin(),placements.end());}return p;
}
inline inventory::Batch open_packets(const inventory::Profile& bank,std::uint32_t npc,bool bags_already_present){storage::validate(bank);validate_presentation_ids(bank);inventory::Batch p;if(!bags_already_present)p=inventory::restore(bank,true);p.push_back(open_window(npc));return p;}
struct Proposal {inventory::Reason status;inventory::Profile character,bank;inventory::Batch packets;bool accepted()const{return status==inventory::Reason::accepted;}};
// Root must commit BOTH owned records with BOTH expected revisions in one
// database transaction before sending this packet batch. No single-row save.
inline Proposal transfer(const inventory::Profile& character,const inventory::Profile& bank,const inventory::Request& request,const Policy& policy){
    inventory::validate_character(character);storage::validate(bank);
    auto reject=[&](inventory::Reason reason){return Proposal{reason,character,bank,{}};};
    for(const auto* p:{&character,&bank})for(const auto& [id,item]:p->items)if(id>=container_first&&id<=container_last)return reject(inventory::Reason::unsupported);
    if(!policy.chest_open)return reject(inventory::Reason::mode);
    if(character.inventory!=bank.inventory)return reject(inventory::Reason::unsupported);
    if(request.change!=inventory::Change::move&&request.change!=inventory::Change::split)return reject(inventory::Reason::unsupported);
    const bool deposit=character.items.contains(request.item),destination_is_bank=bank.bags.contains(request.bag);
    const auto& source=deposit?character:bank;const auto& destination=destination_is_bank?bank:character;
    if(!source.items.contains(request.item))return reject(inventory::Reason::missing);
    if(deposit&&!destination_is_bank)return reject(inventory::Reason::bag);
    if(!(deposit?policy.can_deposit:destination_is_bank?policy.can_deposit&&policy.can_withdraw:policy.can_withdraw))return reject(inventory::Reason::mode);
    if(!destination.bags.contains(request.bag)||(destination_is_bank?
       destination.bags.at(request.bag).type!=4&&destination.bags.at(request.bag).type!=5:destination.bags.at(request.bag).type!=1))return reject(inventory::Reason::bag);
    const auto source_bag=source.bags.find(source.items.at(request.item).bag);
    if(source_bag==source.bags.end()||(deposit?source_bag->second.type!=1:source_bag->second.type!=4&&source_bag->second.type!=5))return reject(inventory::Reason::unsupported);
    // Session IDs must be disjoint across the two loaded ownership domains.
    // A collision needs a session presentation remap, not guessing ownership.
    for(const auto& [id,item]:character.items)if(bank.items.contains(id))return reject(inventory::Reason::unsupported);
    inventory::Profile combined=character;
    combined.material_quota=bank.material_quota;
    combined.next_item=std::max(character.next_item,bank.next_item);
    for(auto [id,b]:bank.bags){if(combined.bags.contains(id))return reject(inventory::Reason::bag);combined.bags.emplace(id,b);}
    // Preserve native storage indexes; this permission is supplied only after
    // chest, direction and ownership checks above, never by a client field.
    for(auto [id,item]:bank.items)combined.items.emplace(id,item);
    // Transient two-domain view can exceed the durable512 item bound. Neither
    // codec nor final character/bank validation receives this widened scope.
    auto proposed=inventory::propose(combined,request,{true,false,true,policy.character},inventory::Bounds::combined_transfer);
    if(!proposed.accepted())return reject(proposed.status);
    // A whole-item swap crosses ownership in BOTH directions, regardless of
    // the requested item's direction. Authorize every actual crossing.
    for(const auto& [id,item]:proposed.profile.items){
        if(character.items.contains(id)&&bank.bags.contains(item.bag)&&!policy.can_deposit)
            return reject(inventory::Reason::mode);
        if(bank.items.contains(id)&&character.bags.contains(item.bag)&&!policy.can_withdraw)
            return reject(inventory::Reason::mode);
    }
    for(const auto& [id,item]:proposed.profile.items)if(id>=container_first&&id<=container_last)return reject(inventory::Reason::unsupported);
    auto next_character=character,next_bank=bank;next_character.items.clear();next_bank.items.clear();
    next_character.next_item=next_bank.next_item=proposed.profile.next_item;
    next_character.weapons=proposed.profile.weapons;
    for(auto [id,item]:proposed.profile.items){if(bank.bags.contains(item.bag))next_bank.items.emplace(id,item);else next_character.items.emplace(id,item);}
    inventory::validate(next_character);storage::validate(next_bank);
    return {inventory::Reason::accepted,std::move(next_character),std::move(next_bank),std::move(proposed.packets)};
}
}

