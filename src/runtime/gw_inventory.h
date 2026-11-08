#pragma once
#include "gw_item_wire.h"
#include "gw_basic_armor_bonus.h"
#include "gw_item_metadata.h"
#include "gw_weapon_sets.h"
#include "gw_outgoing_frame.h"
#include "gw_material_storage.h"
#include <map>
#include <set>
#include <span>
#include <algorithm>
#include <optional>
#include <string_view>

namespace fw::gw::inventory {
using Packet=items::Packet;using Batch=std::vector<Packet>;
struct Bag {std::uint16_t id=0;std::uint8_t type=1,index=0,capacity=0;std::uint32_t container=0;};
struct ReviewedArmor {std::uint16_t base=0,physical=0,elemental=0;bool operator==(const ReviewedArmor&)const=default;};
struct Item {items::Definition definition;std::uint16_t bag=0;std::uint8_t slot=0;std::optional<std::uint8_t> profession;std::optional<std::u16string> customized;std::optional<ReviewedArmor> reviewed_armor;};
// An installed bag item is owned, but is linked by Bag::container rather than
// placed in the bag's contents. Native318 writes Bag+c / Item+8; sending317 for
// the same item would incorrectly make it an ordinary occupied slot.
inline bool installed(const Item& item){return item.bag==0;}
inline items::Definition presented_definition(const Item& item){
    auto definition=item.definition;
    // Explicit private armor ratings are server-owned, but the retail tooltip
    // reads modifier words. A reviewed private baseline may already contain
    // an applied rune/insignia; that must not erase its base tooltip stats.
    // Captured definitions with an explicit base rating remain unchanged.
    const unsigned armor_slot=definition.type==7?2:definition.type==19?3:definition.type==16?4:definition.type==4?5:definition.type==13?6:0;
    const auto has_property=[&](unsigned property){return std::any_of(definition.modifiers.begin(),definition.modifiers.end(),[&](auto word){return ((word>>20)&0x3ffu)==property;});};
    if(item.reviewed_armor&&item.profession&&*item.profession>=1&&*item.profession<=10&&armor_slot&&!has_property(0x23c)){
        const auto armor=item.reviewed_armor->base;
        if(!armor||armor>160)throw std::invalid_argument("private armor tooltip rating");
        definition.modifiers.push_back(0xa3c80000u|(unsigned(armor)<<8));
        const auto bonus=energy::armor_piece(*item.profession,armor_slot);
        if(bonus.capacity&&!has_property(0x22c))definition.modifiers.push_back(0x62c80000u|(bonus.capacity<<8));
        if(bonus.pips&&!has_property(0x22e))definition.modifiers.push_back(0x62e80000u|(bonus.pips<<8));
    }
    return definition;
}
inline Batch item_packets(const Item& item){
    Batch packets{items::definition_2022(presented_definition(item))};
    if(item.profession)packets.push_back(items::profession_2022(item.definition.id,*item.profession));
    if(item.customized&&!item.customized->empty())packets.push_back(items::customization_2022(item.definition.id,*item.customized));
    return packets;
}
struct Profile {
    std::uint16_t inventory=1;
    std::uint32_t next_item=1;
    std::map<std::uint16_t,Bag> bags;
    std::map<std::uint32_t,Item> items;
    weapon_sets::State weapons;
    std::uint32_t gold=0;
    // Account material-pane quota, explicitly reviewed server state. Character
    // profiles retain the baseline value; no packet infers purchased upgrades.
    std::uint16_t material_quota=materials::default_capacity;
};
inline weapon_sets::Lookup weapon_lookup(const Profile& profile){return [&profile](std::uint32_t id)->std::optional<weapon_sets::Item>{
    const auto found=profile.items.find(id);if(found==profile.items.end())return {};
    const auto bag=profile.bags.find(found->second.bag);if(bag==profile.bags.end()||(bag->second.type!=1&&bag->second.type!=2))return {};
    const auto& d=found->second.definition;return weapon_sets::Item{d.type,d.quantity,d.interaction};};}
inline Batch weapon_packets(const Profile& profile){
    if(std::none_of(profile.bags.begin(),profile.bags.end(),[](const auto& pair){return pair.second.type==2&&pair.second.index==21;}))return {};
    return weapon_sets::restore_2022(profile.inventory,profile.weapons,weapon_lookup(profile));
}
inline constexpr std::size_t maximum_items=512,maximum_bags=21,maximum_blob=256*1024;
// An explicit private new-character baseline, not an import of an official
// character's possessions. No appearance-only preview is turned into an item.
inline Profile baseline(){Profile p;p.bags={{1,{1,1,0,20,0}},{2,{2,2,21,9,0}}};return p;}
enum class Bounds {durable,combined_transfer};
inline void validate(const Profile& p,Bounds bounds=Bounds::durable){
    const bool bank_only=!p.bags.empty()&&std::all_of(p.bags.begin(),p.bags.end(),[](const auto& pair){return pair.second.type==4||pair.second.type==5;});
    if(p.gold>(bank_only?1000000u:100000u))throw std::invalid_argument("inventory gold cap exceeded");
    if(!materials::valid_capacity(p.material_quota))throw std::invalid_argument("invalid reviewed material quota");
    const auto limit=bounds==Bounds::combined_transfer?maximum_items*2:maximum_items;
    if((bounds!=Bounds::durable&&bounds!=Bounds::combined_transfer)||!p.inventory||!p.next_item||p.bags.empty()||p.bags.size()>maximum_bags||p.items.size()>limit)
        throw std::invalid_argument("invalid bounded inventory profile");
    std::set<unsigned> indexes;std::set<std::pair<unsigned,unsigned>> occupied;std::set<std::uint32_t> containers;
    for(auto [key,b]:p.bags){
        if(key!=b.id||!b.id||!b.capacity||b.capacity>64||!indexes.insert(b.index).second||
            !(b.type==1&&b.index<=4||b.type==2&&b.index==21&&b.capacity==9||b.type==4&&b.index>=7&&b.index<=20&&(b.capacity==20||b.capacity==25)||
              b.type==5&&b.index==materials::bag_index&&b.capacity==materials::bag_capacity&&!b.container))
            throw std::invalid_argument("unsupported inventory bag");
        if(b.container){
            const auto found=p.items.find(b.container);
            if(b.type!=1||!containers.insert(b.container).second||found==p.items.end()||
                found->second.definition.type!=3||found->second.definition.quantity!=1||
                found->second.bag!=0||found->second.slot!=0)
                throw std::invalid_argument("bag container is not a unique installed owned bag item");
        }
    }
    for(const auto& [id,item]:p.items){
        const auto bag=p.bags.find(item.bag);
        const bool placement=installed(item)?item.slot==0&&containers.contains(id):
            bag!=p.bags.end()&&item.slot<bag->second.capacity&&occupied.emplace(item.bag,item.slot).second;
        if(id!=item.definition.id||id>=p.next_item||!placement)
            throw std::invalid_argument("invalid inventory placement/identity");
        (void)items::definition_2022(item.definition);
        if(item.reviewed_armor&&(!item.profession||!*item.profession||*item.profession>10||item.definition.quantity!=1||
            item.reviewed_armor->base>200||item.reviewed_armor->physical>100||item.reviewed_armor->elemental>100||
            (item.definition.type!=7&&item.definition.type!=19&&item.definition.type!=16&&item.definition.type!=4&&item.definition.type!=13)))
            throw std::invalid_argument("invalid reviewed armor metadata");
        if(bag!=p.bags.end()&&bag->second.type==5){
            const auto element=materials::element(item.definition);
            if(!element||element->element!=item.slot||!(item.definition.interaction&0x80000)||item.definition.quantity>p.material_quota)
                throw std::invalid_argument("invalid material-pane item/slot/quota");
        }else if(bag!=p.bags.end()&&(bag->second.type==1||bag->second.type==4)&&
                 (item.definition.interaction&0x80000)&&item.definition.quantity>materials::ordinary_stack)
            throw std::invalid_argument("ordinary stack exceeds250");
        if(item.profession&&*item.profession>11)throw std::invalid_argument("invalid persisted item profession");
        if(item.customized&&(item.customized->size()>31||item.customized->find(u'\0')!=std::u16string::npos))throw std::invalid_argument("invalid persisted item customization");
    }
    weapon_sets::validate(p.weapons,weapon_lookup(p));
}
inline void validate_character(const Profile& p){
    validate(p);
    if(p.material_quota!=materials::default_capacity)throw std::invalid_argument("account material quota in character inventory");
    for(const auto& [id,bag]:p.bags)if(bag.type!=1&&bag.type!=2)throw std::invalid_argument("bank bag in character inventory");
}
// FWI1..3 used a normal slot for a referenced container. Migrate only that
// explicit legacy relation, after checking the old placements. FWI4 is strict:
// a placed item is never silently reclassified as installed during decoding.
inline void migrate_legacy_containers(Profile& p){
    std::set<std::uint32_t> containers;std::set<std::pair<unsigned,unsigned>> occupied;
    for(const auto& [id,bag]:p.bags)if(bag.container){
        const auto found=p.items.find(bag.container);
        if(bag.type!=1||!containers.insert(bag.container).second||found==p.items.end()||
            found->second.definition.type!=3||found->second.definition.quantity!=1)
            throw std::invalid_argument("invalid legacy bag container relation");
    }
    for(const auto& [id,item]:p.items){
        const auto bag=p.bags.find(item.bag);
        if(installed(item)){if(item.slot||!containers.contains(id))throw std::invalid_argument("invalid legacy unplaced item");}
        else if(bag==p.bags.end()||item.slot>=bag->second.capacity||!occupied.emplace(item.bag,item.slot).second)
            throw std::invalid_argument("invalid legacy item placement");
    }
    for(auto id:containers){auto& item=p.items.at(id);item.bag=0;item.slot=0;}
}
inline Packet encode(const Profile& p){
    validate(p);const bool armor_metadata=std::any_of(p.items.begin(),p.items.end(),[](const auto& pair){return pair.second.reviewed_armor.has_value();});
    Packet b{'F','W','I',static_cast<std::uint8_t>(armor_metadata?6:5)};items::put(b,p.inventory,2);items::put(b,p.next_item,4);items::put(b,static_cast<std::uint32_t>(p.bags.size()),1);items::put(b,static_cast<std::uint32_t>(p.items.size()),2);
    for(auto [id,bag]:p.bags){items::put(b,id,2);items::put(b,bag.type,1);items::put(b,bag.index,1);items::put(b,bag.capacity,1);items::put(b,bag.container,4);}
    for(const auto& [id,item]:p.items){const auto definition=items::definition_2022(item.definition);
        items::put(b,item.bag,2);items::put(b,item.slot,1);items::put(b,static_cast<std::uint32_t>(definition.size()-2),2);b.insert(b.end(),definition.begin()+2,definition.end());
        items::put(b,(item.profession?1u:0u)|(item.customized?2u:0u)|(item.reviewed_armor?4u:0u),1);
        if(item.profession)items::put(b,*item.profession,1);
        if(item.customized){items::put(b,static_cast<unsigned>(item.customized->size()),1);for(auto unit:*item.customized)items::put(b,unit,2);}
        if(item.reviewed_armor){items::put(b,item.reviewed_armor->base,2);items::put(b,item.reviewed_armor->physical,2);items::put(b,item.reviewed_armor->elemental,2);}}
    for(const auto& set:p.weapons.sets){items::put(b,set.weapon,4);items::put(b,set.offhand,4);}items::put(b,p.weapons.active,1);
    items::put(b,p.gold,4);
    items::put(b,p.material_quota,2);
    if(b.size()>maximum_blob)throw std::invalid_argument("inventory blob exceeds bound");return b;
}
inline Profile decode(std::span<const std::uint8_t> b){
    if(b.size()<13||b.size()>maximum_blob||b[0]!='F'||b[1]!='W'||b[2]!='I'||b[3]<1||b[3]>6)
        throw std::invalid_argument("invalid inventory format");
    std::size_t at=4;auto read=[&](unsigned bytes){if(at+bytes>b.size())throw std::invalid_argument("truncated inventory");std::uint32_t v=0;for(unsigned i=0;i<bytes;++i)v|=std::uint32_t(b[at++])<<(8*i);return v;};
    Profile p;p.inventory=static_cast<std::uint16_t>(read(2));p.next_item=read(4);const auto bags=read(1),count=read(2);
    if(!bags||bags>maximum_bags||count>maximum_items)throw std::invalid_argument("inventory count exceeds bound");
    for(unsigned i=0;i<bags;++i){Bag bag;bag.id=static_cast<std::uint16_t>(read(2));bag.type=static_cast<std::uint8_t>(read(1));bag.index=static_cast<std::uint8_t>(read(1));bag.capacity=static_cast<std::uint8_t>(read(1));bag.container=read(4);
        if(!p.bags.emplace(bag.id,bag).second)throw std::invalid_argument("duplicate bag identity");}
    for(unsigned i=0;i<count;++i){Item item;item.bag=static_cast<std::uint16_t>(read(2));item.slot=static_cast<std::uint8_t>(read(1));const auto size=read(2);const auto end=at+size;
        if(size<34||end>b.size())throw std::invalid_argument("invalid inventory item size");auto& d=item.definition;
        d.id=read(4);d.file=read(4);d.type=static_cast<std::uint8_t>(read(1));d.unknown1=static_cast<std::uint8_t>(read(1));d.dye=static_cast<std::uint16_t>(read(2));d.materials=static_cast<std::uint16_t>(read(2));d.unknown2=static_cast<std::uint8_t>(read(1));
        d.interaction=read(4);d.price=read(4);d.model=read(4);d.quantity=read(4);
        const auto chars=read(2);if(chars>63)throw std::invalid_argument("inventory name exceeds bound");for(unsigned n=0;n<chars;++n)d.encoded_name+=static_cast<char16_t>(read(2));
        const auto mods=read(1);if(mods>64)throw std::invalid_argument("inventory modifiers exceed bound");for(unsigned n=0;n<mods;++n)d.modifiers.push_back(read(4));
        if(at!=end)throw std::invalid_argument("inventory item framing");
        if(b[3]>=2){const auto flags=read(1);if(flags>(b[3]>=6?7u:3u))throw std::invalid_argument("inventory metadata flags");
            if(flags&1)item.profession=static_cast<std::uint8_t>(read(1));
            if(flags&2){const auto length=read(1);if(length>31)throw std::invalid_argument("inventory customization length");item.customized.emplace();for(unsigned n=0;n<length;++n)item.customized->push_back(static_cast<char16_t>(read(2)));}
            if(flags&4)item.reviewed_armor=ReviewedArmor{static_cast<std::uint16_t>(read(2)),static_cast<std::uint16_t>(read(2)),static_cast<std::uint16_t>(read(2))};}
        if(!p.items.emplace(d.id,std::move(item)).second)throw std::invalid_argument("inventory duplicate item");
    }
    if(b[3]<=3)migrate_legacy_containers(p);
    if(b[3]>=3){for(auto& set:p.weapons.sets){set.weapon=read(4);set.offhand=read(4);}p.weapons.active=static_cast<std::uint8_t>(read(1));}
    else for(const auto& [id,item]:p.items){const auto found=p.bags.find(item.bag);if(found==p.bags.end())continue;const auto& bag=found->second;if(bag.type==2&&bag.index==21&&item.slot<=1&&weapon_sets::valid_type(item.definition.type,item.slot==0?weapon_sets::Hand::weapon:weapon_sets::Hand::offhand))
        (item.slot==0?p.weapons.sets[0].weapon:p.weapons.sets[0].offhand)=id;}
    if(b[3]>=4)p.gold=read(4);
    if(b[3]>=5)p.material_quota=static_cast<std::uint16_t>(read(2));
    if(at!=b.size())throw std::invalid_argument("inventory trailing data");validate(p);return p;
}
inline Batch restore(const Profile& p,bool inventory_exists=true){
    validate(p);Batch packets;if(!inventory_exists)packets.push_back(items::inventory_2022(p.inventory));
    for(const auto& [id,item]:p.items){auto metadata=item_packets(item);packets.insert(packets.end(),metadata.begin(),metadata.end());}
    for(auto [id,bag]:p.bags)packets.push_back(items::bag_2022(p.inventory,bag.type,bag.index,id,bag.capacity,bag.container));
    for(const auto& [id,item]:p.items)if(!installed(item))packets.push_back(items::location_2022(p.inventory,id,item.bag,item.slot));auto sets=weapon_packets(p);packets.insert(packets.end(),sets.begin(),sets.end());return packets;
}
enum class Change {move,split,equip,unequip,drop,destroy,use};
struct Request {Change change=Change::move;std::uint32_t item=0,quantity=0;std::uint16_t bag=0;std::uint8_t slot=0;std::size_t consumed=0;std::optional<std::uint8_t> equipment_slot;};
inline std::optional<std::uint32_t> owned_request_item(const Profile& profile,const Request& request){
    if(request.change!=Change::unequip)return profile.items.contains(request.item)?std::optional(request.item):std::nullopt;
    if(!request.equipment_slot||*request.equipment_slot>=9)return {};
    for(const auto& [id,item]:profile.items){const auto found=profile.bags.find(item.bag);if(found==profile.bags.end())continue;const auto& bag=found->second;
        if(bag.type==2&&bag.index==21&&item.slot==*request.equipment_slot)return id;}
    return {};
}
inline world::FrameState parse(std::span<const std::uint8_t> bytes,Request& result){
    if(bytes.size()<2)return world::FrameState::incomplete;const auto opcode=(bytes[0]|(unsigned(bytes[1])<<8))&0x7fff;
    if(opcode!=42&&opcode!=46&&opcode!=77&&opcode!=103&&opcode!=112&&opcode!=115&&opcode!=124)return world::FrameState::unknown_opcode;
    const auto frame=world::outgoing_frame_2022(bytes);if(frame.state!=world::FrameState::complete)return frame.state;
    auto read=[&](unsigned at,unsigned size){std::uint32_t v=0;for(unsigned i=0;i<size;++i)v|=std::uint32_t(bytes[at+i])<<(8*i);return v;};
    Request r;r.consumed=frame.size;
    if(opcode==77){r.change=Change::unequip;r.equipment_slot=static_cast<std::uint8_t>(read(2,1));r.bag=static_cast<std::uint16_t>(read(3,2));r.slot=static_cast<std::uint8_t>(read(5,1));}
    else {r.item=read(2,4);
        if(opcode==42){r.change=Change::drop;r.quantity=read(6,1);}
        else if(opcode==46)r.change=Change::equip;
        else if(opcode==103)r.change=Change::destroy;
        else if(opcode==124)r.change=Change::use;
        else if(opcode==112){r.change=Change::move;r.bag=static_cast<std::uint16_t>(read(6,2));r.slot=static_cast<std::uint8_t>(read(8,1));}
        else {r.change=Change::split;r.quantity=read(6,4);r.bag=static_cast<std::uint16_t>(read(10,2));r.slot=static_cast<std::uint8_t>(read(12,1));}}
    result=r;return world::FrameState::complete;
}
struct Policy {bool can_move=false,can_destroy=false,storage_transfer=false;std::u16string character;};
inline bool mutation_allowed(bool in_scene,bool active_trade){return in_scene&&!active_trade;}
enum class Reason {accepted,missing,mode,bag,slot,quantity,full,unsupported,identity_exhausted};
inline const char* reason_text(Reason r){switch(r){case Reason::accepted:return "accepted";case Reason::missing:return "item not owned";case Reason::mode:return "inventory change unavailable";case Reason::bag:return "bag unavailable";case Reason::slot:return "slot unavailable";case Reason::quantity:return "quantity unavailable";case Reason::full:return "destination stack full";case Reason::unsupported:return "item effect/equipment/world-drop unsupported";case Reason::identity_exhausted:return "inventory item limit reached";}return "inventory change rejected";}
struct Proposal {Reason status;Profile profile;Batch packets;bool accepted()const{return status==Reason::accepted;}};
inline bool compatible(const items::Definition& a,const items::Definition& b){
    return (a.interaction&0x80000)&&a.file==b.file&&a.type==b.type&&a.unknown1==b.unknown1&&a.dye==b.dye&&a.materials==b.materials&&a.unknown2==b.unknown2&&
        a.interaction==b.interaction&&a.price==b.price&&a.model==b.model&&a.encoded_name==b.encoded_name&&a.modifiers==b.modifiers;
}
// Equipment_Pack.html permits weapons, armor and costumes. Native bag index4
// identifies the pack independently of its allocated bag ID. ItemType values
// are documented by the local GWCA Constants.h; customization ownership is
// checked separately below, never inferred from item type/name.
inline bool accepts_item_type(const Bag& bag,unsigned type){
    if(bag.type!=1||bag.index!=4)return true;
    if(weapon_sets::valid_type(type,weapon_sets::Hand::weapon)||weapon_sets::valid_type(type,weapon_sets::Hand::offhand))return true;
    switch(type){case 4:case 7:case 13:case 16:case 19:case 44:case 45:return true;default:return false;}
}
inline bool accepts_item(const Bag& bag,const Item& item,std::u16string_view character){
    if(bag.type!=1||bag.index!=4)return true;
    // Absent owner metadata is unknown; explicit empty means uncustomized.
    return accepts_item_type(bag,item.definition.type)&&item.customized&&
        (item.customized->empty()||(!character.empty()&&*item.customized==character));
}
inline Proposal propose(const Profile& current,const Request& r,const Policy& policy,Bounds bounds=Bounds::durable){
    auto finish=[&](Reason status){return Proposal{status,current,{}};};
    if(bounds==Bounds::combined_transfer&&(!policy.storage_transfer||(r.change!=Change::move&&r.change!=Change::split)))return finish(Reason::mode);
    validate(current,bounds);
    if(!policy.can_move)return finish(Reason::mode);
    if(r.change==Change::equip||r.change==Change::unequip||r.change==Change::use||r.change==Change::drop)return finish(Reason::unsupported);
    if(!current.items.contains(r.item))return finish(Reason::missing);
    Profile next=current;Batch packets;auto& source=next.items.at(r.item);
    const auto movable_bag=[&](const Bag& bag){return bag.type==1||(policy.storage_transfer&&(bag.type==4||bag.type==5));};
    const auto source_bag=current.bags.find(source.bag);
    if(source_bag==current.bags.end()||!movable_bag(source_bag->second))return finish(Reason::unsupported);
    if(r.change==Change::destroy){if(!policy.can_destroy)return finish(Reason::mode);for(auto [id,bag]:current.bags)if(bag.container==r.item)return finish(Reason::unsupported);packets.push_back(items::detach_2022(current.inventory,r.item));packets.push_back(items::delete_2022(r.item));next.items.erase(r.item);next.weapons=weapon_sets::erased(next.weapons,r.item);if(next.weapons!=current.weapons){auto sets=weapon_packets(next);packets.insert(packets.end(),sets.begin(),sets.end());}validate(next);return {Reason::accepted,std::move(next),std::move(packets)};}
    if(!next.bags.contains(r.bag)||!movable_bag(next.bags.at(r.bag)))return finish(Reason::bag);
    if(r.slot>=next.bags.at(r.bag).capacity)return finish(Reason::slot);
    const auto requested=r.change==Change::split?r.quantity:source.definition.quantity;
    if(!requested||requested>source.definition.quantity)return finish(Reason::quantity);
    if(source.bag==r.bag&&source.slot==r.slot)return finish(Reason::accepted);
    auto target=std::find_if(next.items.begin(),next.items.end(),[&](const auto& entry){return entry.second.bag==r.bag&&entry.second.slot==r.slot;});
    const auto& destination_bag=next.bags.at(r.bag);
    if(!accepts_item(destination_bag,source,policy.character))return finish(Reason::slot);
    if(source_bag->second.type==5||destination_bag.type==5){
        const auto source_element=materials::element(source.definition);
        if(!source_element||destination_bag.type==5&&source_element->element!=r.slot)return finish(Reason::slot);
        std::optional<Item> destination;if(target!=next.items.end())destination=target->second;
        const auto material=materials::transfer(source,destination,requested,
            {source_bag->second.type==5?current.material_quota:materials::ordinary_stack,
             destination_bag.type==5?current.material_quota:materials::ordinary_stack});
        if(!material.accepted())return finish(material.status==materials::Reason::full?Reason::full:
            material.status==materials::Reason::quantity?Reason::quantity:Reason::slot);
        if(target!=next.items.end()){
            target->second.definition.quantity=material.destination_after;
            packets.push_back(items::quantity_2022(target->first,material.destination_after));
            if(material.source_after){source.definition.quantity=material.source_after;packets.push_back(items::quantity_2022(r.item,material.source_after));}
            else{packets.push_back(items::detach_2022(current.inventory,r.item));packets.push_back(items::delete_2022(r.item));next.items.erase(r.item);}
        }else if(!material.source_after){source.bag=r.bag;source.slot=r.slot;packets.push_back(items::location_2022(current.inventory,r.item,r.bag,r.slot,true));}
        else{
            if(next.items.size()>=(bounds==Bounds::combined_transfer?maximum_items*2:maximum_items)||next.next_item==UINT32_MAX)return finish(Reason::identity_exhausted);
            Item split=source;split.definition.id=next.next_item++;split.definition.quantity=material.moved;split.bag=r.bag;split.slot=r.slot;
            source.definition.quantity=material.source_after;packets.push_back(items::quantity_2022(r.item,material.source_after));
            auto metadata=item_packets(split);packets.insert(packets.end(),metadata.begin(),metadata.end());
            packets.push_back(items::location_2022(current.inventory,split.definition.id,r.bag,r.slot));next.items.emplace(split.definition.id,std::move(split));
        }
        validate(next,bounds);return {Reason::accepted,std::move(next),std::move(packets)};
    }
    if(target!=next.items.end()){
        auto& destination=target->second;
        if(source.profession==destination.profession&&source.customized==destination.customized&&compatible(source.definition,destination.definition)){
            if(destination.definition.quantity>=250)return finish(Reason::full);
            const auto transferred=std::min(requested,250-destination.definition.quantity);source.definition.quantity-=transferred;destination.definition.quantity+=transferred;
            packets.push_back(items::quantity_2022(target->first,destination.definition.quantity));
            if(source.definition.quantity)packets.push_back(items::quantity_2022(r.item,source.definition.quantity));
            else {packets.push_back(items::detach_2022(current.inventory,r.item));packets.push_back(items::delete_2022(r.item));next.items.erase(r.item);}
        }else {
            if(requested!=source.definition.quantity)return finish(Reason::slot);
            if(!accepts_item(source_bag->second,destination,policy.character))return finish(Reason::slot);
            const auto old_bag=source.bag;const auto old_slot=source.slot;
            packets.push_back(items::detach_2022(current.inventory,target->first));
            source.bag=r.bag;source.slot=r.slot;destination.bag=old_bag;destination.slot=old_slot;
            packets.push_back(items::location_2022(current.inventory,r.item,source.bag,source.slot,true));
            packets.push_back(items::location_2022(current.inventory,target->first,destination.bag,destination.slot));
        }
    }else if(requested==source.definition.quantity){source.bag=r.bag;source.slot=r.slot;packets.push_back(items::location_2022(current.inventory,r.item,r.bag,r.slot,true));}
    else {
        if(!(source.definition.interaction&0x80000))return finish(Reason::quantity);
        if(next.items.size()>=(bounds==Bounds::combined_transfer?maximum_items*2:maximum_items)||next.next_item==UINT32_MAX)return finish(Reason::identity_exhausted);
        Item split=source;split.definition.id=next.next_item++;split.definition.quantity=requested;split.bag=r.bag;split.slot=r.slot;source.definition.quantity-=requested;
        packets.push_back(items::quantity_2022(r.item,source.definition.quantity));auto metadata=item_packets(split);packets.insert(packets.end(),metadata.begin(),metadata.end());
        packets.push_back(items::location_2022(current.inventory,split.definition.id,r.bag,r.slot));next.items.emplace(split.definition.id,std::move(split));
    }
    for(const auto& set:current.weapons.sets)for(auto id:{set.weapon,set.offhand})if(id&&!weapon_lookup(next)(id))next.weapons=weapon_sets::erased(next.weapons,id);
    if(next.weapons!=current.weapons){auto sets=weapon_packets(next);packets.insert(packets.end(),sets.begin(),sets.end());}
    validate(next,bounds);return {Reason::accepted,std::move(next),std::move(packets)};
}
// Reassert only an existing item's location/count after well-framed denial. A
// whole inventory replay would assert occupied slots without first detaching.
inline Batch reconcile(const Profile& current,std::uint32_t item){
    if(!current.items.contains(item))return {};const auto& value=current.items.at(item);if(installed(value))return {};
    return {items::quantity_2022(item,value.definition.quantity),items::location_2022(current.inventory,item,value.bag,value.slot,true)};
}
}
