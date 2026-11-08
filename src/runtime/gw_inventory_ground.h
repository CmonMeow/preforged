#pragma once
#include "gw_inventory_identity.h"
#include "gw_ground_items.h"
#include <tuple>

namespace fw::gw::inventory::ground {
enum class Reason {accepted,mode,missing,carrying,container,quantity,full,identity,metadata,restricted,bundle};
inline const char* reason_text(Reason reason){switch(reason){
    case Reason::accepted:return "accepted";case Reason::mode:return "mode";case Reason::missing:return "item_not_owned";
    case Reason::carrying:return "carrying_item_required";case Reason::container:return "active_bag_container";
    case Reason::quantity:return "quantity";case Reason::full:return "inventory_full";
    case Reason::identity:return "owned_wire_identity_exhausted";case Reason::metadata:return "invalid_item_metadata";
    case Reason::restricted:return "item_drop_restricted";
    case Reason::bundle:return "held_bundle_path_required";
}return "invalid";}
inline bool wire_view(const Profile& current){
    inventory::validate(current);
    if(current.inventory!=identity::local_inventory||current.next_item>identity::owned_last+1)return false;
    for(const auto& [id,item]:current.items)if(!id||id>identity::owned_last)return false;
    return true;
}
struct DropProposal {
    Reason status;Profile profile;Batch packets;Item source;unsigned quantity=0;
    bool accepted()const{return status==Reason::accepted;}
};
// Original2022 gmview4dadc0 drag kind5. Tradeability is a different rule.
inline bool drop_permitted(std::uint32_t interaction,bool explorable,bool cinematic){
    return explorable&&!cinematic&&!(interaction&0x100u);
}
// Prepare only. Caller independently authorizes map mode/cinematic state and
// commits this owned profile's durable projection inside world's drop CAS.
// World receives the exact original source definition/metadata, not a preview
// or an appearance. A dropped active weapon-set reference is actually removed.
inline DropProposal debit(const Profile& current,std::uint32_t wire,unsigned quantity,bool allowed){
    const auto reject=[&](Reason reason){return DropProposal{reason,current,{},{},0};};
    if(!wire_view(current))return reject(Reason::identity);
    if(!allowed)return reject(Reason::mode);
    const auto found=current.items.find(wire);if(found==current.items.end())return reject(Reason::missing);
    const auto& source=found->second;
    if(source.definition.interaction&0x100u)return reject(Reason::restricted);
    for(const auto& [id,bag]:current.bags)if(bag.container==wire)return reject(Reason::container);
    if(!current.bags.contains(source.bag)||current.bags.at(source.bag).type!=1)return reject(Reason::carrying);
    if(!quantity||quantity>255||quantity>source.definition.quantity)return reject(Reason::quantity);
    Profile proposed=current;Batch packets;
    if(quantity==source.definition.quantity){
        packets.push_back(items::detach_2022(current.inventory,wire));packets.push_back(items::delete_2022(wire));
        proposed.items.erase(wire);proposed.weapons=weapon_sets::erased(proposed.weapons,wire);
        if(proposed.weapons!=current.weapons){auto sets=weapon_packets(proposed);packets.insert(packets.end(),sets.begin(),sets.end());}
    }else{
        proposed.items.at(wire).definition.quantity-=quantity;
        packets.push_back(items::quantity_2022(wire,proposed.items.at(wire).definition.quantity));
    }
    inventory::validate(proposed);return {Reason::accepted,std::move(proposed),std::move(packets),source,quantity};
}
struct CreditProposal {
    Reason status;Profile profile;Batch packets;unsigned quantity=0;
    bool accepted()const{return status==Reason::accepted;}
};
// All requested units fit, or none are credited. Only carrying slots/stacks
// participate. World separately validates live scope, ownership, range, plane,
// source lifetime and serializes contenders before invoking the durable CAS.
// Never persist floor.definition.id: allocate a new low-pool owned identity.
inline CreditProposal credit(const Profile& current,const world::ground::Item& floor,unsigned quantity,bool allowed,std::u16string_view character={}){
    const auto reject=[&](Reason reason){return CreditProposal{reason,current,{},0};};
    if(!wire_view(current))return reject(Reason::identity);
    if(!allowed)return reject(Reason::mode);
    if(floor.definition.type==6)return reject(Reason::bundle);
    if(!quantity||quantity>floor.definition.quantity)return reject(Reason::quantity);
    try{(void)items::definition_2022(floor.definition);}catch(const std::invalid_argument&){return reject(Reason::metadata);}
    if(!world::ground::valid_scope(floor.scope)||!world::ground::valid_scope(floor.source)||
       floor.scope.instance!=floor.source.instance||!floor.source_item||!world::ground::valid_position(floor.position)||
       !world::ground::valid_reviewed_armor(floor)||
       floor.profession&&*floor.profession>11||floor.customized&&(floor.customized->size()>31||floor.customized->find(u'\0')!=floor.customized->npos))
        return reject(Reason::metadata);
    Profile proposed=current;Batch packets;unsigned remaining=quantity;
    const bool stackable=(floor.definition.interaction&0x80000)!=0;
    Item incoming;incoming.definition=floor.definition;incoming.customized=floor.customized;
    // Native bag indexes define the UI ordering, not arbitrary durable/wire IDs.
    std::vector<std::uint16_t> carrying;
    for(const auto& [id,bag]:current.bags)if(bag.type==1&&accepts_item(bag,incoming,character))carrying.push_back(id);
    std::sort(carrying.begin(),carrying.end(),[&](auto a,auto b){return std::tie(current.bags.at(a).index,a)<std::tie(current.bags.at(b).index,b);});
    if(stackable){
        std::vector<std::uint32_t> merge;
        for(const auto& [id,item]:current.items)
            if(current.bags.contains(item.bag)&&current.bags.at(item.bag).type==1&&accepts_item(current.bags.at(item.bag),incoming,character)&&item.definition.quantity<250&&item.profession==floor.profession&&
               item.customized==floor.customized&&item.reviewed_armor==floor.reviewed_armor&&inventory::compatible(item.definition,floor.definition))merge.push_back(id);
        std::sort(merge.begin(),merge.end(),[&](auto a,auto b){const auto& x=current.items.at(a);const auto& y=current.items.at(b);
            return std::tie(current.bags.at(x.bag).index,x.slot,a)<std::tie(current.bags.at(y.bag).index,y.slot,b);});
        for(auto id:merge){auto& target=proposed.items.at(id);const auto added=std::min(remaining,250-target.definition.quantity);
            target.definition.quantity+=added;remaining-=added;packets.push_back(items::quantity_2022(id,target.definition.quantity));
            if(!remaining)break;}
    }
    for(auto bag:carrying){
        for(unsigned slot=0;remaining&&slot<proposed.bags.at(bag).capacity;++slot){
            if(std::any_of(proposed.items.begin(),proposed.items.end(),[&](const auto& pair){return pair.second.bag==bag&&pair.second.slot==slot;}))continue;
            if(proposed.items.size()>=maximum_items)return reject(Reason::full);
            if(proposed.next_item>identity::owned_last)return reject(Reason::identity);
            Item added;added.definition=floor.definition;added.definition.id=proposed.next_item++;
            added.definition.quantity=stackable?std::min(remaining,250u):remaining;
            added.profession=floor.profession;added.customized=floor.customized;added.reviewed_armor=floor.reviewed_armor;added.bag=bag;added.slot=static_cast<std::uint8_t>(slot);
            remaining-=added.definition.quantity;
            auto definitions=item_packets(added);packets.insert(packets.end(),definitions.begin(),definitions.end());
            packets.push_back(items::location_2022(current.inventory,added.definition.id,bag,added.slot));
            proposed.items.emplace(added.definition.id,std::move(added));
        }
        if(!remaining)break;
    }
    if(remaining)return reject(Reason::full);
    inventory::validate(proposed);return {Reason::accepted,std::move(proposed),std::move(packets),quantity};
}
}
