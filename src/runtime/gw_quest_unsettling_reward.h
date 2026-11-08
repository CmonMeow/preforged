#pragma once
#include "gw_quest_unsettling_rumors.h"
#include "gw_inventory.h"

namespace fw::gw::quests::unsettling_rumors {
// Exact checkpoint-1815 item564874/location564875/removal564876/XP564881.
// Source SHA256 e210b492c044e475735ac55dca85944598fff445e120e657674df9d7fc4df55c.
// Preserve opaque modifiers and unknown customization status.
inline items::Definition tall_shield(std::uint32_t identity){
    items::Definition item;
    item.id=identity;item.file=2147493176u;item.type=24;item.interaction=536870912;
    item.price=10;item.model=2817;item.quantity=1;
    item.encoded_name={9809,33833,54950,12855};
    item.modifiers={0x21a83205,0x23480f00,0xa3c80400};
    return item;
}
struct RewardProposal {Profile quests;inventory::Profile inventory;inventory::Batch packets;unsigned item=0,xp=experience;};
// Persist BOTH profiles and XP using one revision-checked transaction. Publish
// packets only after commit. Full bags, replay and stale lifetimes grant nothing.
inline std::optional<RewardProposal> reward(const Profile& quests,const inventory::Profile& items,
    unsigned cycle,const Eligibility& eligibility,const Interaction& npc){
    inventory::validate_character(items);
    auto proposed=plan(quests,Action::reward,cycle,eligibility,npc,true);
    if(!proposed||items.next_item>=65535||items.items.size()>=inventory::maximum_items)return {};
    inventory::Item reward_item;reward_item.definition=tall_shield(items.next_item);
    // The captured reward burst establishes no customization metadata. Keep
    // it unknown; do not invent an uncustomized item to admit it into a pack.
    for(const auto& [id,bag]:items.bags){
        if(bag.type!=1||!inventory::accepts_item(bag,reward_item,{}))continue;
        for(unsigned slot=0;slot<bag.capacity;++slot){
            if(std::any_of(items.items.begin(),items.items.end(),[&](const auto& row){return row.second.bag==id&&row.second.slot==slot;}))continue;
            RewardProposal out{*proposed,items};auto item=reward_item;
            out.item=out.inventory.next_item++;
            item.bag=id;item.slot=static_cast<std::uint8_t>(slot);
            out.packets=inventory::item_packets(item);
            out.packets.push_back(items::location_2022(items.inventory,out.item,id,item.slot));
            out.inventory.items.emplace(out.item,std::move(item));
            inventory::validate_character(out.inventory);return out;
        }
    }
    return {};
}
}
