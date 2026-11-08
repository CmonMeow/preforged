#pragma once
#include "network.h"
#include "gw_quest_timed_state.h"
#include "gw_arena_wire.h"
#include "gw_quest_wire.h"
#include "gw_inventory_gold.h"
#include "gw_presearing_items.h"
#include "gw_inventory_identity.h"
#include "gw_quest_skills.h"
#include "gw_quest_wayward_reward.h"
#include "gw_quest_unsettling_reward.h"
#include "gw_quest_opposition.h"
#include <set>
namespace gwserver {
struct QuestActionResult {std::uint32_t status;std::string message;std::optional<fw::gw::items::Packet> packet;std::uint32_t committed_cycle=0;fw::gw::quests::Batch skill_packets;};
struct QuestAcceptanceResult {std::uint32_t status;std::string message;fw::gw::quests::Batch packets;};
struct QuestInventoryGrant {SaveInventory durable;fw::gw::quests::Batch packets;};
struct QuestItemRewardResult {
    std::uint32_t status;std::string message;fw::gw::quests::Batch packets;
    std::optional<fw::gw::inventory::identity::Profiles> profiles;
};
// Session-exclusive operation: construct the identity projection and publication
// here, before the durable CAS. No caller-provided packets or durable item IDs.
inline QuestItemRewardResult rewardCharacterQuestItem(NetworkComponent& endpoint,RewardQuestExperience request,
    fw::gw::inventory::identity::Session& identities,const fw::gw::inventory::Profile& proposed_wire,
    const fw::gw::inventory::Profile& bank_wire){
    if(request.reward_inventory||request.gold||request.consume_shimmering_scale||!request.learned_skills.empty()||
       request.fill_skillbar||request.unlock_trial||!request.id||!request.cycle)
        return {FW_INVALID,"item reward requires one prepared item and XP only",{}, {}};
    try{
        const auto current=endpoint.characterInventory(request.owner);
        if(current.status!=FW_OK)return {current.status,current.message,{}, {}};
        if(!current.record)return {FW_NOT_FOUND,"reward inventory missing",{}, {}};
        if(current.record->revision!=request.expected_inventory_revision)return {FW_CONFLICT,"reward inventory revision changed",{}, {}};
        auto prepared=identities.prepare(proposed_wire,bank_wire);
        if(!identities.preserves_bank(prepared))return {FW_INVALID,"item reward cannot change account storage",{}, {}};
        auto retained=prepared.durable.character;const auto& before=current.record->profile;
        const auto added=retained.items.find(before.next_item);
        if(added==retained.items.end()||before.items.contains(before.next_item)||retained.next_item!=before.next_item+1||
           added->second.definition.quantity!=1||!retained.bags.contains(added->second.bag)||retained.bags.at(added->second.bag).type!=1)
            return {FW_INVALID,"reward must add one carried item",{}, {}};
        retained.items.erase(added);retained.next_item=before.next_item;
        if(fw::gw::inventory::encode(retained)!=fw::gw::inventory::encode(before))return {FW_CONFLICT,"reward changed existing inventory",{}, {}};
        fw::gw::quests::Batch packets;unsigned introduced=0;
        for(const auto& [id,item]:prepared.wire.character.items)if(!identities.origin(id)){
            ++introduced;packets=fw::gw::inventory::item_packets(item);
            packets.push_back(fw::gw::items::location_2022(fw::gw::inventory::identity::local_inventory,id,item.bag,item.slot));
        }
        if(introduced!=1)return {FW_INVALID,"reward identity projection is not one new item",{}, {}};
        packets.push_back(fw::gw::quests::remove_2022(request.id));
        request.reward_inventory=prepared.durable.character;
        const auto saved=endpoint.rewardQuestExperience(request);
        if(saved.status!=FW_OK)return {saved.status,saved.message,{}, {}};
        if(!identities.commit(std::move(prepared)))throw std::runtime_error("committed reward identity projection became obsolete");
        return {FW_OK,{},std::move(packets),std::move(prepared.wire)};
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}, {}};}
}

inline QuestAcceptanceResult advanceWaywardWizard(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::wayward_wizard::Eligibility& eligibility,
    const fw::gw::quests::wayward_wizard::Interaction& npc,fw::gw::quests::wayward_wizard::Action action){
    namespace wizard=fw::gw::quests::wayward_wizard;
    if(action!=wizard::Action::talk_ralena&&action!=wizard::Action::talk_orion)return {FW_INVALID,"not a dialogue objective",{}};
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"Wayward Wizard not active",{}};
    const auto proposed=wizard::plan(loaded.record->profile,action,cycle,eligibility,npc);
    if(!proposed)return {FW_CONFLICT,"Wayward Wizard dialogue changed",{}};
    try{
        const auto found=catalog.find(wizard::id);if(found==catalog.end())return {FW_INVALID,"Wayward Wizard content missing",{}};
        const auto& progress=proposed->records.at(wizard::id);
        const auto& content=fw::gw::quests::progress_content(found->second,progress);
        if(!content.description||!content.objectives)return {FW_INVALID,"Wayward Wizard details missing",{}};
        fw::gw::quests::Batch packets;
        packets.push_back(fw::gw::quests::objectives_2022(wizard::id,*content.objectives));
        if(progress.stage==fw::gw::quests::Stage::completed)packets.push_back(fw::gw::quests::marker_2022(content.entry));
        const auto saved=endpoint.saveQuests({owner,loaded.record->revision,*proposed});
        if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
        return {FW_OK,{},std::move(packets)};
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
}
inline QuestAcceptanceResult advanceUnsettlingRumors(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::unsettling_rumors::Eligibility& eligibility,
    const fw::gw::quests::unsettling_rumors::Interaction& npc,fw::gw::quests::unsettling_rumors::Action action){
    namespace wizard=fw::gw::quests::unsettling_rumors;
    if(action!=wizard::Action::talk_meerak&&action!=wizard::Action::talk_armin)return {FW_INVALID,"not a dialogue objective",{}};
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"Unsettling Rumors not active",{}};
    const auto proposed=wizard::plan(loaded.record->profile,action,cycle,eligibility,npc);
    if(!proposed)return {FW_CONFLICT,"Unsettling Rumors dialogue changed",{}};
    try{
        const auto found=catalog.find(wizard::id);if(found==catalog.end())return {FW_INVALID,"Unsettling Rumors content missing",{}};
        const auto& progress=proposed->records.at(wizard::id);
        const auto& content=fw::gw::quests::progress_content(found->second,progress);
        if(!content.description||!content.objectives)return {FW_INVALID,"Unsettling Rumors details missing",{}};
        fw::gw::quests::Batch packets;
        packets.push_back(fw::gw::quests::objectives_2022(wizard::id,*content.objectives));
        packets.push_back(fw::gw::quests::marker_2022(content.entry));
        const auto saved=endpoint.saveQuests({owner,loaded.record->revision,*proposed});
        if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
        return {FW_OK,{},std::move(packets)};
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
}
inline QuestAcceptanceResult questionOppositionWitness(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,unsigned cycle,const fw::gw::quests::opposition::Eligibility& eligibility,
    const fw::gw::quests::opposition::Interaction& npc){
    namespace opposition=fw::gw::quests::opposition;
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"Opposition not active",{}};
    const auto proposed=opposition::plan(loaded.record->profile,opposition::Action::question,cycle,eligibility,npc);
    if(!proposed)return {FW_CONFLICT,"Opposition witness changed or already questioned",{}};
    try{
        const auto found=catalog.find(opposition::id);if(found==catalog.end())return {FW_INVALID,"Opposition content missing",{}};
        const auto& content=fw::gw::quests::progress_content(found->second,proposed->records.at(opposition::id));
        if(!content.description||!content.objectives)return {FW_INVALID,"Opposition details missing",{}};
        fw::gw::quests::Batch packets{fw::gw::quests::objectives_2022(opposition::id,*content.objectives),fw::gw::quests::marker_2022(content.entry)};
        const auto saved=endpoint.saveQuests({owner,loaded.record->revision,*proposed});
        if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
        return {FW_OK,{},std::move(packets)};
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
}
// Trusted NPC controller supplies reviewed grants. Prepare owner-only learned
// notifications before commit; a competing skill mutation invalidates the CAS.
inline QuestAcceptanceResult rewardCharacterQuest(NetworkComponent& endpoint,const RewardQuestExperience& request,const fw::gw::inventory::identity::Session* identities=nullptr,std::uint32_t actor=0){
    if(request.reward_inventory)return {FW_INVALID,"item reward requires prepared inventory publication",{}};
    if(request.fill_skillbar&&(!actor||request.learned_skills.empty()))return {FW_INVALID,"reward slots require actor and skills",{}};
    if(request.unlock_trial&&!actor)return {FW_INVALID,"profession unlock requires owned actor",{}};
    if(request.consume_shimmering_scale&&!identities)return {FW_INVALID,"scale turn-in requires session item identities",{}};
    if(!request.id||!request.cycle)return {FW_INVALID,"quest identity and attempt required",{}};
    fw::gw::quests::Batch packets;
    try{
        if(request.gold||request.consume_shimmering_scale){
            const auto inventory=endpoint.characterInventory(request.owner);
            if(inventory.status!=FW_OK)return {inventory.status,inventory.message,{}};
            if(!inventory.record)return {FW_NOT_FOUND,"quest inventory baseline missing",{}};
            if(inventory.record->revision!=request.expected_inventory_revision)return {FW_CONFLICT,"quest inventory revision changed",{}};
            fw::gw::inventory::validate_character(inventory.record->profile);
            if(request.gold>100000u-inventory.record->profile.gold)return {FW_CONFLICT,"quest gold exceeds carrying capacity",{}};
            // Owner's native inventory must already match this revision. The
            // caller advances its session balance/revision after publication.
            if(request.gold)packets.push_back(fw::gw::inventory::gold::delta_2022(319,inventory.record->profile.inventory,request.gold));
            if(request.consume_shimmering_scale){
                if(request.id!=83)return {FW_INVALID,"scale turn-in requires Elementalist Test",{}};
                const auto consumed=fw::gw::inventory::presearing::consume_shimmering_scale(inventory.record->profile);
                if(!consumed)return {FW_CONFLICT,"Shimmering Scale not carried",{}};
                for(const auto& [id,item]:inventory.record->profile.items){
                    const auto after=consumed->items.find(id);
                    const auto wire=identities->wire_item(fw::gw::inventory::identity::Domain::character,id);
                    if(!wire)return {FW_CONFLICT,"quest inventory identity missing",{}};
                    if(after==consumed->items.end()){
                        packets.push_back(fw::gw::items::detach_2022(fw::gw::inventory::identity::local_inventory,*wire));
                        packets.push_back(fw::gw::items::delete_2022(*wire));
                    }else if(after->second.definition.quantity!=item.definition.quantity)
                        packets.push_back(fw::gw::items::quantity_2022(*wire,after->second.definition.quantity));
                }
            }
        }
        if(!request.learned_skills.empty()||request.unlock_trial){
            const auto loaded=endpoint.characterSkills(request.owner);
            if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
            if(!loaded.record)return {FW_NOT_FOUND,"quest skill baseline missing",{}};
            if(loaded.record->revision!=request.expected_skill_revision)return {FW_CONFLICT,"quest skill revision changed",{}};
            auto proposed=fw::gw::quests::teach(loaded.record->profile,request.learned_skills,request.fill_skillbar);
            if(request.unlock_trial){proposed=fw::gw::quests::unlock_trial(proposed,request.id);
                packets.push_back(fw::gw::skills::profession_packets_2022(actor,proposed.primary,proposed.secondary,proposed.profession_mask)[1]);}
            const auto& learned=proposed.learned;
            packets.push_back(fw::gw::skills::learned_packet_2022(learned));
            for(auto id:request.learned_skills){
                const bool newly_learned=!loaded.record->profile.learned.contains(id);
                packets.push_back(newly_learned?fw::gw::skills::learning_packet_2022(id,learned.at(id)):
                    fw::gw::skills::learned_count_packet_2022(id,learned.at(id)));
            }
            if(request.fill_skillbar)for(unsigned slot=0;slot<proposed.bar.size();++slot)
                if(proposed.bar[slot]!=loaded.record->profile.bar[slot])
                    packets.push_back(fw::gw::skills::slot_packet_2022(actor,slot,proposed.bar[slot]));
        }
        packets.push_back(fw::gw::quests::remove_2022(request.id));
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
    const auto saved=endpoint.rewardQuestExperience(request);
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
// XP-only rewards: callers must reject rewards containing gold/items/skills
// until those changes can participate in the same transaction.
inline QuestActionResult rewardCharacterQuestExperience(NetworkComponent& endpoint,const RewardQuestExperience& request){
    if(request.reward_inventory)return {FW_INVALID,"item reward requires prepared inventory publication",{}};
    if(request.unlock_trial)return {FW_INVALID,"profession unlock requires typed state publication",{}};
    if(request.gold||request.consume_shimmering_scale)return {FW_INVALID,"inventory rewards require inventory-state publication",{}};
    if(!request.id||!request.cycle)return {FW_INVALID,"quest identity and attempt required",{}};
    if(!request.learned_skills.empty())return {FW_INVALID,"skill rewards require learned-state publication",{}};
    const auto packet=fw::gw::quests::remove_2022(request.id);
    const auto saved=endpoint.rewardQuestExperience(request);
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},packet};
}
// Internal controller entry point. The caller must establish an authorized NPC
// offer and prerequisites; neither id nor repeatability comes from client trust.
// Prepare all wire data before persisting, and publish only after the CAS commit.
inline QuestAcceptanceResult acceptCharacterQuest(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,std::uint32_t id,bool repeatable,std::span<const std::uint32_t> grants={},
    const std::optional<QuestInventoryGrant>& inventory={},std::uint32_t actor=0,bool trial=false,std::optional<std::uint64_t> timed_start_utc_ms={}){
    if(!id)return {FW_INVALID,"quest identity required",{}};
    if((id==42)!=timed_start_utc_ms.has_value())return {FW_INVALID,"timed quest requires its authoritative start",{}};
    if(trial&&(!actor||grants.empty()))return {FW_INVALID,"trial requires owned actor and grants",{}};
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    const auto current=loaded.record?loaded.record->profile:fw::gw::quests::Profile{};
    auto proposed=fw::gw::quests::accept(current,id,repeatable);
    if(!proposed)return {FW_CONFLICT,"quest cannot be accepted",{}};
    if(timed_start_utc_ms){proposed=fw::gw::quests::arm_marksmanship(*proposed,proposed->records.at(id).cycle,*timed_start_utc_ms);
        if(!proposed)return {FW_INVALID,"invalid timed quest start",{}};}
    fw::gw::quests::Profile newly_active;newly_active.records.emplace(id,proposed->records.at(id));
    fw::gw::quests::Batch packets;
    try{packets=fw::gw::quests::restore_2022(newly_active,catalog);}
    catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
    if(inventory){
        if(inventory->durable.owner.account.bytes!=owner.account.bytes||inventory->durable.owner.character.bytes!=owner.character.bytes||inventory->packets.empty())
            return {FW_INVALID,"acceptance item requires matching owner and prepared publication",{}};
        packets.insert(packets.end(),inventory->packets.begin(),inventory->packets.end());
    }
    std::uint64_t skill_revision=0;
    if(!grants.empty()){
        const auto skills=endpoint.characterSkills(owner);
        if(skills.status!=FW_OK)return {skills.status,skills.message,{}};
        if(!skills.record)return {FW_NOT_FOUND,"acceptance skill baseline missing",{}};
        skill_revision=skills.record->revision;
        fw::gw::skills::Profile proposed;
        try{proposed=fw::gw::quests::teach_acceptance(skills.record->profile,id,grants,actor!=0,trial);}
        catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
        const auto& learned=proposed.learned;
        packets.push_back(fw::gw::skills::learned_packet_2022(learned));
        for(auto skill:grants)packets.push_back(skills.record->profile.learned.contains(skill)?
            fw::gw::skills::learned_count_packet_2022(skill,learned.at(skill)):
            fw::gw::skills::learning_packet_2022(skill,learned.at(skill)));
        if(actor)for(unsigned slot=0;slot<proposed.bar.size();++slot)
            if(proposed.bar[slot]!=skills.record->profile.bar[slot])
                packets.push_back(fw::gw::skills::slot_packet_2022(actor,slot,proposed.bar[slot]));
    }
    SaveQuests acceptance{owner,loaded.record?loaded.record->revision:0,*proposed,skill_revision,{grants.begin(),grants.end()}};
    acceptance.acceptance_fill_skillbar=actor!=0&&!grants.empty();
    acceptance.acceptance_trial=trial;
    if(inventory){acceptance.expected_inventory_revision=inventory->durable.expected_revision;acceptance.acceptance_inventory=inventory->durable.profile;}
    const auto saved=endpoint.saveQuests(acceptance);
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
inline QuestActionResult readCharacterQuest(NetworkComponent& endpoint,const OwnedCharacter& owner,const fw::gw::quests::Catalog& catalog,std::uint32_t id,bool marker){
    const auto loaded=endpoint.characterQuests(owner);if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"quest not active",{}};
    if(marker){const auto reply=fw::gw::quests::marker_reply_2022(loaded.record->profile,catalog,{id,true});
        return {static_cast<std::uint32_t>(reply.packet?FW_OK:FW_NOT_FOUND),reply.packet?"":"quest marker unavailable",reply.packet};}
    const auto owned=loaded.record->profile.records.find(id);
    if(owned==loaded.record->profile.records.end()||(owned->second.stage!=fw::gw::quests::Stage::active&&owned->second.stage!=fw::gw::quests::Stage::completed))return {FW_NOT_FOUND,"quest not active",{}};
    const auto content=catalog.find(id);
    if(content==catalog.end()||content->second.entry.id!=id||!content->second.description||!content->second.objectives)return {FW_NOT_FOUND,"quest details unavailable",{}};
    try{
        const auto& selected=fw::gw::quests::progress_content(content->second,owned->second);
        if(!selected.description||!selected.objectives)return {FW_NOT_FOUND,"quest details unavailable",{}};
        return {FW_OK,{},fw::gw::quests::details_2022(id,*selected.description,*selected.objectives)};
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
}
// Called only after the server objective controller proves completion for this
// attempt. It does not authorize a client to choose its own completion or reward.
inline QuestAcceptanceResult creditCharacterQuest(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,std::uint32_t id,std::uint32_t cycle,std::uint64_t credit,unsigned required,std::optional<std::uint64_t> occurred_utc_ms={}){
    const auto found=catalog.find(id);
    if(found==catalog.end()||!required||found->second.objective_variants.size()+1!=required||!found->second.completed_variant)
        return {FW_INVALID,"counted quest content unavailable",{}};
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"counted quest not active",{}};
    const auto progress=loaded.record->profile.records.find(id);
    if(progress==loaded.record->profile.records.end()||progress->second.cycle!=cycle||
       (progress->second.stage!=fw::gw::quests::Stage::active&&progress->second.stage!=fw::gw::quests::Stage::completed))
        return {FW_CONFLICT,"counted quest attempt changed",{}};
    if(progress->second.objective_credits.contains(credit))return {FW_OK,{},{}};
    const auto proposed=fw::gw::quests::credit_objective(loaded.record->profile,id,cycle,credit,required,occurred_utc_ms);
    if(!proposed)return {FW_CONFLICT,"counted quest credit rejected",{}};
    fw::gw::quests::Batch packets;
    try{
        const auto& content=fw::gw::quests::progress_content(found->second,proposed->records.at(id));
        if(!content.description||!content.objectives)throw std::invalid_argument("counted quest details unavailable");
        packets.push_back(fw::gw::quests::objectives_2022(id,*content.objectives));
        if(proposed->records.at(id).stage==fw::gw::quests::Stage::completed)
            packets.push_back(fw::gw::quests::marker_2022(content.entry));
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
    const auto saved=endpoint.saveQuests({owner,loaded.record->revision,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
inline QuestAcceptanceResult expireCharacterMarksmanship(NetworkComponent& endpoint,const OwnedCharacter& owner,
    std::uint32_t cycle,std::uint64_t now_utc_ms){
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"timed quest not active",{}};
    const auto found=loaded.record->profile.records.find(42);
    if(found==loaded.record->profile.records.end()||found->second.cycle!=cycle)return {FW_CONFLICT,"timed quest attempt changed",{}};
    if(found->second.timed&&found->second.timed->expired)return {FW_OK,{},{}};
    const auto proposed=fw::gw::quests::expire_marksmanship(loaded.record->profile,cycle,now_utc_ms);
    if(!proposed)return {FW_CONFLICT,"timed quest is not eligible for expiry",{}};
    const auto saved=endpoint.saveQuests({owner,loaded.record->revision,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{}, {fw::gw::arena::clear_clock_2022()}};
}
inline QuestAcceptanceResult completeCharacterQuest(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,std::uint32_t id,std::uint32_t cycle){
    if(!id||!cycle)return {FW_INVALID,"quest identity and attempt required",{}};
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"quest not active",{}};
    const auto proposed=fw::gw::quests::advance(loaded.record->profile,id,cycle,fw::gw::quests::Stage::active,fw::gw::quests::Stage::completed);
    if(!proposed)return {FW_CONFLICT,"quest attempt cannot complete",{}};
    const auto found=catalog.find(id);
    if(found==catalog.end()||found->second.entry.id!=id)return {FW_INVALID,"quest content unavailable",{}};
    const auto& content=fw::gw::quests::stage_content(found->second,fw::gw::quests::Stage::completed);
    fw::gw::quests::Batch packets;
    try{
        if(!content.description||!content.objectives)throw std::invalid_argument("quest completion objectives unavailable");
        packets.push_back(fw::gw::quests::objectives_2022(id,*content.objectives));
        packets.push_back(fw::gw::quests::marker_2022(content.entry));
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
    const auto saved=endpoint.saveQuests({owner,loaded.record->revision,*proposed});
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},std::move(packets)};
}
// Collection readiness is read from authoritative carried inventory, never a
// client completion request. Completion does not consume the trophy; reward
// turn-in rechecks it and consumes it in the XP/skills transaction.
inline QuestAcceptanceResult completeElementalistTest(NetworkComponent& endpoint,const OwnedCharacter& owner,
    const fw::gw::quests::Catalog& catalog,std::uint32_t cycle){
    if(!cycle||!catalog.contains(83))return {FW_INVALID,"Elementalist Test content unavailable",{}};
    const auto inventory=endpoint.characterInventory(owner);
    if(inventory.status!=FW_OK)return {inventory.status,inventory.message,{}};
    if(!inventory.record)return {FW_NOT_FOUND,"quest inventory baseline missing",{}};
    try{
        if(!fw::gw::inventory::presearing::carried_shimmering_scale(inventory.record->profile))
            return {FW_CONFLICT,"Shimmering Scale not carried",{}};
    }catch(const std::invalid_argument& e){return {FW_INVALID,e.what(),{}};}
    return completeCharacterQuest(endpoint,owner,catalog,83,cycle);
}
inline QuestActionResult abandonCharacterQuest(NetworkComponent& endpoint,const OwnedCharacter& owner,std::uint32_t id,std::uint32_t actor=0){
    if(!id)return {FW_INVALID,"quest identity required",{}};
    const auto loaded=endpoint.characterQuests(owner);
    if(loaded.status!=FW_OK)return {loaded.status,loaded.message,{}};
    if(!loaded.record)return {FW_NOT_FOUND,"quest not active",{}};
    const auto found=loaded.record->profile.records.find(id);
    if(found==loaded.record->profile.records.end())return {FW_NOT_FOUND,"quest not active",{}};
    const auto proposed=fw::gw::quests::abandon(loaded.record->profile,id,found->second.cycle);
    if(!proposed)return {FW_CONFLICT,"quest cannot be abandoned",{}};
    SaveQuests request{owner,loaded.record->revision,*proposed};fw::gw::quests::Batch skill_packets;
    const auto skills=endpoint.characterSkills(owner);
    if(skills.status!=FW_OK)return {skills.status,skills.message,{}};
    if(skills.record&&!skills.record->profile.trial_skills.empty()){
        if(!actor)return {FW_INVALID,"trial abandonment requires owned actor",{}};
        const auto pruned=fw::gw::quests::prune_trials(skills.record->profile,*proposed);
        request.expected_skill_revision=skills.record->revision;request.abandon_trial_quest=id;
        for(unsigned slot=0;slot<pruned.bar.size();++slot)if(pruned.bar[slot]!=skills.record->profile.bar[slot])
            skill_packets.push_back(fw::gw::skills::slot_packet_2022(actor,slot,pruned.bar[slot]));
    }
    const auto saved=endpoint.saveQuests(request);
    if(saved.status!=FW_OK)return {saved.status,saved.message,{}};
    return {FW_OK,{},fw::gw::quests::remove_2022(id),found->second.cycle,std::move(skill_packets)};
}
}
