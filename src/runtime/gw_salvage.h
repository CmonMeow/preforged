#pragma once
#include "gw_identification.h"
#include "gw_item_transactions.h"

namespace fw::gw::salvage {
enum class Mode {materials,component};
// Supplied by a reviewed outcome provider, never inferred from item names.
// Full snapshot binding prevents applying an outcome to altered possessions.
struct Outcome {
    inventory::Packet expected_inventory;
    Mode mode=Mode::materials;
    bool reviewed=false,rare_material=false;
    std::vector<inventory::Item> outputs;
    std::optional<inventory::Item> retained;
};
struct Pending {
    std::uint32_t kit=0,target=0;std::uint64_t revision=0,expires=0;
    bool current(std::uint64_t rev,std::uint64_t now)const{return kit&&target&&revision==rev&&now<expires;}
    void clear(){*this={};}
};
enum class Reason {accepted,missing,kit,outcome,capacity,target};
struct Proposal {Reason status;inventory::Profile profile;inventory::Batch packets;bool accepted()const{return status==Reason::accepted;}};
inline Proposal apply(const inventory::Profile& current,std::uint32_t kit_id,std::uint32_t target_id,
                      const Outcome& outcome,const identification::Policy& policy,
                      inventory::Bounds bounds=inventory::Bounds::durable){
    auto deny=[&](Reason r){return Proposal{r,current,{}};};
    inventory::validate(current,bounds);
    const auto kit=current.items.find(kit_id),target=current.items.find(target_id);
    if(!kit_id||kit_id==target_id||kit==current.items.end()||target==current.items.end())return deny(Reason::missing);
    const auto bag=current.bags.find(target->second.bag);
    // Local Wiki Equipment_Pack.html: contents cannot be salvaged in place.
    // The container's native index identifies the pack, not its allocated ID.
    if(bag==current.bags.end()||bag->second.type!=1||bag->second.index==4||target->second.definition.quantity!=1)return deny(Reason::target);
    const auto decoded=items::modifiers::reviewed_kit(items::modifiers::decode_wire(kit->second.definition.modifiers));
    if(!decoded||decoded->kind==items::modifiers::Kit::identification)return deny(Reason::kit);
    if(outcome.mode==Mode::materials&&(decoded->kind==items::modifiers::Kit::perfect_salvage||
       (outcome.rare_material&&decoded->kind==items::modifiers::Kit::basic_salvage)))return deny(Reason::kit);
    if(outcome.mode==Mode::component&&decoded->kind==items::modifiers::Kit::basic_salvage)return deny(Reason::kit);
    if(!outcome.reviewed||outcome.expected_inventory!=inventory::encode(current)||outcome.outputs.empty()||outcome.outputs.size()>3)return deny(Reason::outcome);
    if(outcome.mode==Mode::materials&&outcome.retained)return deny(Reason::outcome);
    if(decoded->kind==items::modifiers::Kit::perfect_salvage&&!outcome.retained)return deny(Reason::outcome);
    if(outcome.retained){
        auto copy=target->second;copy.definition.modifiers=outcome.retained->definition.modifiers;
        inventory::Profile expected=current,actual=current;
        expected.items.at(target_id)=copy;actual.items.at(target_id)=*outcome.retained;
        if(inventory::encode(expected)!=inventory::encode(actual))return deny(Reason::outcome);
    }
    // Require genuinely empty carried slots BEFORE removing target or kit.
    // Equipment packs hold gear only, never the material/component outputs.
    std::vector<std::pair<std::uint16_t,std::uint8_t>> slots;
    for(const auto& [id,b]:current.bags)if(b.type==1&&b.index!=4)for(unsigned slot=0;slot<b.capacity;++slot){
        bool occupied=false;for(const auto& [item_id,item]:current.items)if(item.bag==id&&item.slot==slot)occupied=true;
        if(!occupied)slots.emplace_back(id,static_cast<std::uint8_t>(slot));
    }
    if(slots.size()<outcome.outputs.size())return deny(Reason::capacity);
    auto debit=identification::debit_kit(current,kit_id,decoded->kind,policy,bounds);
    if(!debit.accepted())return deny(Reason::kit);
    Proposal result{Reason::accepted,std::move(debit.profile),std::move(debit.packets)};
    if(outcome.retained){result.profile.items.at(target_id)=*outcome.retained;auto p=inventory::item_packets(*outcome.retained);result.packets.insert(result.packets.end(),p.begin(),p.end());}
    else{result.profile.items.erase(target_id);result.profile.weapons=weapon_sets::erased(result.profile.weapons,target_id);
        result.packets.push_back(items::detach_2022(current.inventory,target_id));result.packets.push_back(items::delete_2022(target_id));}
    for(unsigned i=0;i<outcome.outputs.size();++i){
        auto output=outcome.outputs[i];
        if(output.definition.id||output.bag||output.customized||output.reviewed_armor||
           output.definition.type!=(outcome.mode==Mode::materials?11:8)||!output.definition.quantity||
           result.profile.next_item==UINT32_MAX)return deny(Reason::outcome);
        output.definition.id=result.profile.next_item++;output.bag=slots[i].first;output.slot=slots[i].second;
        auto packets=inventory::item_packets(output);result.packets.insert(result.packets.end(),packets.begin(),packets.end());
        result.packets.push_back(items::location_2022(current.inventory,output.definition.id,output.bag,output.slot));
        result.profile.items.emplace(output.definition.id,std::move(output));
    }
    try{inventory::validate(result.profile,bounds);}catch(const std::invalid_argument&){return deny(Reason::outcome);}
    if(result.profile.weapons!=current.weapons){auto packets=inventory::weapon_packets(result.profile);
        result.packets.insert(result.packets.end(),packets.begin(),packets.end());}
    return result;
}
}
