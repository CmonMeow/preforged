#pragma once
#include "quest_actions.h"
#include "gw_quest_ranger_companion.h"
namespace gwserver {
inline QuestAcceptanceResult acceptRangerCompanion(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::ranger_companion::Eligibility& eligibility,
    const fw::gw::quests::ranger_companion::Interaction& npc,unsigned actor,
    fw::gw::inventory::identity::Session& identities,fw::gw::inventory::Profile& character,
    fw::gw::inventory::Profile& bank,std::uint64_t& inventory_revision){
    namespace ranger=fw::gw::quests::ranger_companion;
    if(!actor)return {FW_INVALID,"Ranger Companion requires owned actor",{}};
    const auto quests=endpoint.characterQuests(owner);if(quests.status!=FW_OK)return {quests.status,quests.message,{}};
    const auto pets=endpoint.characterPets(owner);if(pets.status!=FW_OK)return {pets.status,pets.message,{}};
    const auto skills=endpoint.characterSkills(owner);if(skills.status!=FW_OK)return {skills.status,skills.message,{}};
    const auto owned=endpoint.characterInventory(owner);if(owned.status!=FW_OK)return {owned.status,owned.message,{}};
    if(!pets.record||!skills.record||!owned.record)return {FW_NOT_FOUND,"Ranger Companion baseline unavailable",{}};
    if(owned.record->revision!=inventory_revision||skills.record->profile.primary!=eligibility.primary||skills.record->profile.secondary!=eligibility.secondary)
        return {FW_CONFLICT,"Ranger Companion baseline changed",{}};
    const auto current=quests.record?quests.record->profile:fw::gw::quests::Profile{};
    const auto proposed=ranger::accept(current,cycle,eligibility,pets.record->profile,npc);
    if(!proposed)return {FW_CONFLICT,"Ranger Companion acceptance unavailable or existing pet",{}};
    try{
        const auto content=catalog.find(ranger::id);
        if(content==catalog.end()||content->second.entry.id!=ranger::id||!content->second.description||!content->second.objectives||
           !content->second.completed_variant||content->second.completed_variant->entry.id!=ranger::id||
           !content->second.completed_variant->description||!content->second.completed_variant->objectives)
            return {FW_INVALID,"Ranger Companion required active/completed content missing",{}};
        const auto bow=ranger::grant_bow(character);if(!bow)return {FW_CONFLICT,"Ranger Companion bow inventory full",{}};
        if(bow->item&&*bow->item>fw::gw::inventory::identity::owned_last)return {FW_CONFLICT,"Ranger Companion item identities exhausted",{}};
        auto prepared=identities.prepare(bow->profile,bank);
        if(!identities.preserves_bank(prepared))return {FW_INVALID,"Ranger Companion changed bank",{}};
        const auto durable_bow=ranger::grant_bow(owned.record->profile);
        if(!durable_bow||fw::gw::inventory::encode(durable_bow->profile)!=fw::gw::inventory::encode(prepared.durable.character))
            return {FW_CONFLICT,"Ranger Companion wire/durable bow projection differs",{}};
        const bool trial=eligibility.primary!=2&&!eligibility.secondary;
        const auto taught=fw::gw::quests::teach_acceptance(skills.record->profile,ranger::id,ranger::grants,true,trial);
        fw::gw::quests::Batch packets=bow->packets;
        packets.push_back(fw::gw::skills::learned_packet_2022(taught.learned));
        for(auto skill:ranger::grants)if(!skills.record->profile.learned.contains(skill))
            packets.push_back(fw::gw::skills::learning_packet_2022(skill,taught.learned.at(skill)));
        for(unsigned slot=0;slot<taught.bar.size();++slot)if(taught.bar[slot]!=skills.record->profile.bar[slot])
            packets.push_back(fw::gw::skills::slot_packet_2022(actor,slot,taught.bar[slot]));
        fw::gw::quests::Profile active;active.records.emplace(ranger::id,proposed->records.at(ranger::id));
        auto added=fw::gw::quests::restore_2022(active,catalog);packets.insert(packets.end(),added.begin(),added.end());
        SaveQuests request{owner,quests.record?quests.record->revision:0,*proposed,skills.record->revision,
            {ranger::grants.begin(),ranger::grants.end()},inventory_revision};
        request.acceptance_fill_skillbar=true;request.acceptance_trial=trial;
        if(bow->item)request.acceptance_inventory=prepared.durable.character;
        const auto saved=endpoint.saveQuests(request);if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
        if(!identities.commit(std::move(prepared)))throw std::runtime_error("committed Ranger Companion projection obsolete");
        character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);if(bow->item)++inventory_revision;
        return {FW_OK,{},std::move(packets)};
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
}
inline QuestAcceptanceResult completeRangerCompanion(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,bool prophecies,bool in_presearing){
    namespace ranger=fw::gw::quests::ranger_companion;
    const auto quests=endpoint.characterQuests(owner);if(quests.status!=FW_OK)return {quests.status,quests.message,{}};
    if(!quests.record)return {FW_NOT_FOUND,"Ranger Companion not active",{}};
    const auto found=quests.record->profile.records.find(ranger::id);
    if(found==quests.record->profile.records.end()||found->second.cycle!=cycle)return {FW_CONFLICT,"Ranger Companion attempt changed",{}};
    if(found->second.stage!=fw::gw::quests::Stage::active)return {FW_OK,{}, {}};
    const auto skills=endpoint.characterSkills(owner);const auto pets=endpoint.characterPets(owner);
    if(skills.status!=FW_OK)return {skills.status,skills.message,{}};
    if(pets.status!=FW_OK)return {pets.status,pets.message,{}};
    if(!skills.record||!pets.record)return {FW_NOT_FOUND,"Ranger Companion completion baseline missing",{}};
    const auto& build=skills.record->profile;
    const auto proposed=ranger::complete(quests.record->profile,cycle,{prophecies,in_presearing,build.primary,build.secondary},pets.record->profile);
    if(!proposed)return {FW_OK,{}, {}};
    fw::gw::quests::Batch packets;
    try{
        const auto content=catalog.find(ranger::id);
        if(content==catalog.end()||!content->second.completed_variant||content->second.completed_variant->entry.id!=ranger::id||
           !content->second.completed_variant->description||!content->second.completed_variant->objectives)
            throw std::invalid_argument("Ranger Companion completion content missing");
        // Capture30352 lines16446/16447 at82072343: modern77/76 maps to
        // native2022 marker76/details75; the completed snapshot is line1864.
        packets={fw::gw::quests::marker_2022(content->second.completed_variant->entry),
            fw::gw::quests::details_2022(ranger::id,*content->second.completed_variant->description,*content->second.completed_variant->objectives)};
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
    const auto saved=endpoint.saveQuests({owner,quests.record->revision,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
inline QuestAcceptanceResult rewardRangerCompanion(NetworkComponent& endpoint,const OwnedCharacter& owner,unsigned cycle,
    const fw::gw::quests::ranger_companion::Eligibility& eligibility,const fw::gw::quests::ranger_companion::Interaction& npc,unsigned actor){
    namespace ranger=fw::gw::quests::ranger_companion;
    const auto quests=endpoint.characterQuests(owner);const auto pets=endpoint.characterPets(owner);const auto skills=endpoint.characterSkills(owner);
    if(quests.status!=FW_OK)return {quests.status,quests.message,{}};
    if(pets.status!=FW_OK)return {pets.status,pets.message,{}};
    if(skills.status!=FW_OK)return {skills.status,skills.message,{}};
    if(!quests.record||!pets.record||!skills.record||!ranger::reward(quests.record->profile,cycle,eligibility,pets.record->profile,npc))
        return {FW_CONFLICT,"Ranger Companion reward unavailable",{}};
    const bool unlock=skills.record->profile.primary!=2&&!skills.record->profile.secondary;
    RewardQuestExperience request{owner,quests.record->revision,ranger::id,cycle,ranger::experience};
    request.unlock_trial=unlock;if(unlock)request.expected_skill_revision=skills.record->revision;
    return rewardCharacterQuest(endpoint,request,nullptr,actor);
}
}
