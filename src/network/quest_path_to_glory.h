#pragma once
#include "quest_actions.h"
#include "gw_quest_path_to_glory.h"
namespace gwserver {
inline QuestAcceptanceResult acceptPathToGlory(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::path_to_glory::Eligibility& eligibility,
    const fw::gw::quests::path_to_glory::Interaction& npc){
    namespace glory=fw::gw::quests::path_to_glory;
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    const auto proposed=glory::accept(loaded.record?loaded.record->profile:fw::gw::quests::Profile{},cycle,eligibility,npc);
    if(!proposed)return {FW_CONFLICT,"Path to Glory acceptance changed",{}};
    fw::gw::quests::Batch packets;
    try{
        const auto content=catalog.find(glory::id);
        if(content==catalog.end()||!content->second.description||!content->second.objectives||content->second.captured_character_argument)
            throw std::invalid_argument("Path to Glory requires bound catalog details");
        fw::gw::quests::Profile active;active.records.emplace(glory::id,proposed->records.at(glory::id));
        packets=fw::gw::quests::restore_2022(active,catalog);
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
    const auto saved=endpoint.saveQuests({owner,loaded.record?loaded.record->revision:0,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
inline QuestAcceptanceResult rewardPathToGlory(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::path_to_glory::Eligibility& eligibility,
    const fw::gw::quests::path_to_glory::Interaction& npc){
    namespace glory=fw::gw::quests::path_to_glory;
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record||!glory::reward(loaded.record->profile,cycle,eligibility,npc))
        return {FW_CONFLICT,"Path to Glory turn-in changed",{}};
    fw::gw::quests::Batch packets;
    try{
        const auto found=catalog.find(glory::id);
        if(found==catalog.end()||found->second.entry.id!=glory::id||!found->second.description||
           !found->second.objectives||found->second.captured_character_argument)
            throw std::invalid_argument("Path to Glory requires bound active catalog details");
        const auto& content=found->second;
        // Allocate every bounded active-text refresh before any durable write.
        // This is not invented83 completion or duplicate72/79 QuestAdded.
        packets={fw::gw::quests::marker_2022(content.entry),
            fw::gw::quests::details_2022(glory::id,*content.description,*content.objectives),fw::gw::quests::remove_2022(glory::id)};
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
    const auto saved=endpoint.rewardQuestExperience({owner,loaded.record->revision,glory::id,cycle,glory::experience});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
}
