#pragma once
#include "quest_actions.h"
#include "gw_quest_message_from_friend.h"
namespace gwserver {
inline QuestAcceptanceResult acceptMessageFromFriend(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::message_from_friend::Eligibility& eligibility,
    const fw::gw::quests::message_from_friend::Interaction& npc){
    namespace message=fw::gw::quests::message_from_friend;
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    const auto proposed=message::accept(loaded.record?loaded.record->profile:fw::gw::quests::Profile{},cycle,eligibility,npc);
    if(!proposed)return {FW_CONFLICT,"Message from a Friend acceptance changed",{}};
    fw::gw::quests::Batch packets;
    try{
        fw::gw::quests::Profile newly_active;newly_active.records.emplace(message::id,proposed->records.at(message::id));
        packets=fw::gw::quests::restore_2022(newly_active,catalog);
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
    const auto saved=endpoint.saveQuests({owner,loaded.record?loaded.record->revision:0,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
inline QuestAcceptanceResult rewardMessageFromFriend(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,
    const fw::gw::quests::message_from_friend::Eligibility& eligibility,const fw::gw::quests::message_from_friend::Interaction& npc,
    fw::gw::inventory::identity::Session& identities,fw::gw::inventory::Profile& character,
    fw::gw::inventory::Profile& bank,std::uint64_t& inventory_revision){
    namespace message=fw::gw::quests::message_from_friend;
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record||!message::reward(loaded.record->profile,cycle,eligibility,npc))
        return {FW_CONFLICT,"Message from a Friend turn-in changed",{}};
    const auto owned=endpoint.characterInventory(owner);
    if(owned.status!=FW_OK)return {owned.status,owned.message,{}};
    if(!owned.record)return {FW_NOT_FOUND,"Message from a Friend inventory missing",{}};
    if(owned.record->revision!=inventory_revision)return {FW_CONFLICT,"Message from a Friend inventory changed",{}};
    try{
        if(character.gold>100000u-message::gold)return {FW_CONFLICT,"Message from a Friend exceeds carrying capacity",{}};
        auto next=character;next.gold+=message::gold;
        auto prepared=identities.prepare(next,bank);
        if(!identities.preserves_bank(prepared))return {FW_INVALID,"Message from a Friend changed storage",{}};
        auto expected=owned.record->profile;
        if(expected.gold>100000u-message::gold)return {FW_CONFLICT,"Message from a Friend exceeds carrying capacity",{}};
        expected.gold+=message::gold;
        if(fw::gw::inventory::encode(expected)!=fw::gw::inventory::encode(prepared.durable.character))
            return {FW_CONFLICT,"Message from a Friend session inventory differs",{}};
        // Captured marker/details refreshes retain the ACTIVE text. They do
        // not prove retail durable completion or justify invented83 updates.
        // Validate and allocate the entire batch before any durable reward.
        const auto found=catalog.find(message::id);
        if(found==catalog.end()||found->second.entry.id!=message::id||!found->second.description||
           !found->second.objectives||found->second.captured_character_argument)
            return {FW_INVALID,"Message from a Friend requires bound active catalog details",{}};
        const auto& content=found->second;
        fw::gw::quests::Batch packets{fw::gw::quests::marker_2022(content.entry),
            fw::gw::quests::details_2022(message::id,*content.description,*content.objectives),
            fw::gw::quests::remove_2022(message::id),
            fw::gw::inventory::gold::delta_2022(319,character.inventory,message::gold)};
        RewardQuestExperience request{owner,loaded.record->revision,message::id,cycle,message::experience};
        request.gold=message::gold;request.expected_inventory_revision=inventory_revision;
        const auto saved=endpoint.rewardQuestExperience(request);
        if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
        if(!identities.commit(std::move(prepared)))throw std::runtime_error("committed Message reward identity projection obsolete");
        character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);++inventory_revision;
        return {FW_OK,{},std::move(packets)};
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
}
}
