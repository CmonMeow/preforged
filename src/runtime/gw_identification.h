#pragma once
#include "gw_inventory.h"
#include <optional>

namespace fw::gw::identification {
using Batch=inventory::Batch;
using Kit=items::modifiers::Kit;

// These describe a reviewed caller's authority, not facts derived from an item
// name/model. The planner alone cannot reveal an official unidentified drop.
struct Policy {bool allowed=false,reviewed_source=false,storage_access=false;};
enum class Reason {accepted,mode,missing,source,quantity,charges,outcome,target,already_identified};
inline const char* reason_text(Reason value){
    switch(value){case Reason::accepted:return "accepted";case Reason::mode:return "item use unavailable";
    case Reason::missing:return "item not owned";case Reason::source:return "reviewed carried kit required";
    case Reason::quantity:return "single kit quantity required";case Reason::charges:return "authentic kit charges unavailable";
    case Reason::outcome:return "authoritative identification result unavailable";case Reason::target:return "identification result changes item ownership or identity";
    case Reason::already_identified:return "item already identified";}return "identification rejected";
}
struct Proposal {
    Reason status;
    inventory::Profile profile;
    Batch packets;
    unsigned uses_before=0,uses_after=0;
    bool exhausted=false;
    bool accepted()const{return status==Reason::accepted;}
};

// Native351 descriptor60d8e0/callback7c54f0: detail only. This preserves the
// base fields/profession/customization, and does not itself notify item-update.
inline items::Packet detail_2022(const items::Definition& item){
    (void)items::definition_2022(item);
    items::Packet out;items::put(out,351,2);items::put(out,item.id,4);
    items::put(out,item.model,4);items::put(out,item.quantity,4);
    items::put(out,static_cast<unsigned>(item.encoded_name.size()),2);
    for(auto unit:item.encoded_name)items::put(out,unit,2);
    items::put(out,static_cast<unsigned>(item.modifiers.size()),1);
    for(auto modifier:item.modifiers)items::put(out,modifier,4);
    return out;
}

// A bounded charge debit, without an identification/salvage outcome recipe.
// Local2021 Wiki kits are nonstackable. Quantity>1 is deliberately unresolved.
// Raw merchant value remains intact; a future reviewed quote/outcome provider
// must supply any correct resale-value change separately.
inline Proposal debit_kit(const inventory::Profile& current,std::uint32_t source,
                          Kit expected,const Policy& policy,inventory::Bounds bounds=inventory::Bounds::durable){
    auto deny=[&](Reason reason){return Proposal{reason,current,{}};};
    inventory::validate(current,bounds);
    if(!policy.allowed)return deny(Reason::mode);
    const auto found=current.items.find(source);
    if(found==current.items.end())return deny(Reason::missing);
    const auto& original=found->second;
    if(!policy.reviewed_source||original.definition.type!=29||!current.bags.contains(original.bag)||current.bags.at(original.bag).type!=1)
        return deny(Reason::source);
    if(original.definition.quantity!=1)return deny(Reason::quantity);
    const auto decoded=items::modifiers::decode_wire(original.definition.modifiers);
    const auto kit=items::modifiers::reviewed_kit(decoded);
    if(!kit||kit->kind!=expected)return deny(Reason::charges);
    Proposal proposal{Reason::accepted,current,{},kit->uses_each,kit->uses_each-1,kit->uses_each==1};
    if(proposal.exhausted){
        // Quantity0 is not deletion in the retail client.
        proposal.packets.push_back(items::detach_2022(current.inventory,source));
        proposal.packets.push_back(items::delete_2022(source));
        proposal.profile.items.erase(source);
    }else{
        auto& definition=proposal.profile.items.at(source).definition;
        for(auto& modifier:definition.modifiers)if((modifier>>16)==0x2458)
            modifier=(modifier&0xffffff00u)|proposal.uses_after;
        proposal.packets.push_back(detail_2022(definition));
        // Original312 always rebuilds presentation and emits item-update104,
        // even when its quantity is unchanged. No charge is represented as qty.
        proposal.packets.push_back(items::quantity_2022(source,1));
    }
    inventory::validate(proposal.profile,bounds);
    return proposal;
}

inline bool preserves_target(const inventory::Item& current,const inventory::Item& revealed){
    const auto& a=current.definition;const auto& b=revealed.definition;
    return current.bag==revealed.bag&&current.slot==revealed.slot&&
        current.profession==revealed.profession&&current.customized==revealed.customized&&current.reviewed_armor==revealed.reviewed_armor&&
        a.id==b.id&&a.file==b.file&&a.type==b.type&&a.model==b.model&&a.quantity==b.quantity&&
        a.unknown1==b.unknown1&&a.dye==b.dye&&a.materials==b.materials&&a.unknown2==b.unknown2&&
        (a.interaction&~1u)==(b.interaction&~1u)&&(b.interaction&1u)!=0;
}

// authoritative_reveal contains complete, independently known revealed data.
// This API never manufactures hidden modifiers, merchant value or unlocks.
// Commit the proposal using the caller's character/session generation and
// inventory revision/CAS before sending ANY packet. Rejection has no debit.
inline Proposal identify(const inventory::Profile& current,std::uint32_t source,
                         std::uint32_t target,const std::optional<inventory::Item>& authoritative_reveal,
                         const Policy& policy,inventory::Bounds bounds=inventory::Bounds::durable){
    auto deny=[&](Reason reason){return Proposal{reason,current,{}};};
    inventory::validate(current,bounds);
    if(!policy.allowed)return deny(Reason::mode);
    if(!source||!target||source==target)return deny(Reason::target);
    const auto found=current.items.find(target);
    if(found==current.items.end()||!current.items.contains(source))return deny(Reason::missing);
    if(!current.bags.contains(found->second.bag))return deny(Reason::target);
    const auto target_bag=current.bags.at(found->second.bag).type;
    if(target_bag!=1&&target_bag!=2&&!(target_bag==4&&policy.storage_access))
        return deny(Reason::target);
    if(found->second.definition.interaction&1u)return deny(Reason::already_identified);
    if(!authoritative_reveal)return deny(Reason::outcome);
    if(!preserves_target(found->second,*authoritative_reveal))return deny(Reason::target);
    try{(void)items::definition_2022(authoritative_reveal->definition);}
    catch(const std::invalid_argument&){return deny(Reason::outcome);}
    auto proposal=debit_kit(current,source,Kit::identification,policy,bounds);
    if(!proposal.accepted())return proposal;
    proposal.profile.items.at(target)=*authoritative_reveal;
    // Same-ID352 preserves location;353 frees the old represented item and
    // would require a separate proven replacement lifecycle. Reapply metadata.
    auto target_packets=inventory::item_packets(*authoritative_reveal);
    proposal.packets.insert(proposal.packets.end(),target_packets.begin(),target_packets.end());
    inventory::validate(proposal.profile,bounds);
    return proposal;
}
}
