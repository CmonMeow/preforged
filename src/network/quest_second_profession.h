#pragma once
#include "quest_actions.h"
#include "gw_quest_second_profession.h"

namespace gwserver {
inline QuestAcceptanceResult acceptSecondProfession(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::second_profession::Eligibility& eligibility,
    const fw::gw::quests::second_profession::Interaction& npc){
    namespace second=fw::gw::quests::second_profession;
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    const auto current=loaded.record?loaded.record->profile:fw::gw::quests::Profile{};
    const auto proposed=second::plan(current,second::Action::accept,cycle,eligibility,npc);
    if(!proposed)return {FW_CONFLICT,"Second Profession acceptance changed",{}};
    fw::gw::quests::Batch packets;
    try{
        fw::gw::quests::Profile newly_active;newly_active.records.emplace(second::id,proposed->records.at(second::id));
        packets=fw::gw::quests::restore_2022(newly_active,catalog);
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
    const auto saved=endpoint.saveQuests({owner,loaded.record?loaded.record->revision:0,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
// Reconcile after successful profession commit, acceptance or restoration.
// The durable skill profile is authority; caller previews never enter here.
// Storage independently repeats readiness inside its quest CAS transaction.
inline QuestAcceptanceResult completeSecondProfession(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,bool prophecies,bool in_presearing){
    namespace second=fw::gw::quests::second_profession;
    if(!cycle||!prophecies||!in_presearing)return {FW_INVALID,"Second Profession scope unavailable",{}};
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"Second Profession not active",{}};
    const auto found=loaded.record->profile.records.find(second::id);
    if(found==loaded.record->profile.records.end()||found->second.cycle!=cycle)
        return {FW_CONFLICT,"Second Profession attempt changed",{}};
    if(found->second.stage!=fw::gw::quests::Stage::active)return {FW_OK,{}, {}};
    const auto skills=endpoint.characterSkills(owner);
    if(skills.status!=FW_OK)return {skills.status,skills.message,{}};
    if(!skills.record)return {FW_NOT_FOUND,"Second Profession skill baseline missing",{}};
    const auto& profile=skills.record->profile;
    const auto proposed=second::plan(loaded.record->profile,second::Action::complete,cycle,
        {prophecies,in_presearing,profile.primary,profile.secondary});
    if(!proposed)return {FW_OK,{}, {}}; // Not trained/selected yet; leave active.
    fw::gw::quests::Batch packets;
    try{
        const auto content=catalog.find(second::id);
        if(content==catalog.end()||content->second.entry.id!=second::id||!content->second.completed_variant)
            throw std::invalid_argument("Second Profession completion content unavailable");
        const auto& complete=*content->second.completed_variant;
        if(!complete.description||!complete.objectives)throw std::invalid_argument("Second Profession completion details missing");
        packets={fw::gw::quests::objectives_2022(second::id,*complete.objectives),fw::gw::quests::marker_2022(complete.entry)};
    }catch(const std::invalid_argument& error){return {FW_INVALID,error.what(),{}};}
    const auto saved=endpoint.saveQuests({owner,loaded.record->revision,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
}
