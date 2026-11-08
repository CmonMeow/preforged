#pragma once
#include "progression_state.h"
#include "gw_quest_state.h"
#include "inventory_state.h"
namespace gwserver {
using QuestsResult=ProfileResult<fw::gw::quests::Profile>;
struct SaveQuests {OwnedCharacter owner;std::uint64_t expected_revision;fw::gw::quests::Profile profile;
    std::uint64_t expected_skill_revision=0;std::vector<std::uint32_t> acceptance_skills;
    std::uint64_t expected_inventory_revision=0;std::optional<fw::gw::inventory::Profile> acceptance_inventory;
    bool acceptance_fill_skillbar=false;
    bool acceptance_trial=false;
    std::uint32_t abandon_trial_quest=0;};
// Trusted reward controller request, never decoded from client dialogue input.
// XP and the completed->rewarded fence commit in one storage transaction.
struct RewardQuestExperience {OwnedCharacter owner;std::uint64_t expected_revision;std::uint32_t id,cycle,experience;
    std::uint64_t expected_skill_revision=0;std::vector<std::uint32_t> learned_skills;
    std::uint64_t expected_inventory_revision=0;std::uint32_t gold=0;
    bool consume_shimmering_scale=false;
    bool fill_skillbar=false;
    bool unlock_trial=false;
    std::optional<fw::gw::inventory::Profile> reward_inventory;};
inline std::string storage_payload(const SaveQuests& request){
    if(!request.acceptance_skills.empty()||request.acceptance_inventory||request.acceptance_fill_skillbar||request.acceptance_trial||request.abandon_trial_quest)throw std::invalid_argument("quest skill changes require typed transaction");
    check_revision(request.expected_revision);
    return storage_payload(request.owner)+"\t"+std::to_string(request.expected_revision)+"\t"+storage_hex(fw::gw::quests::encode(request.profile));
}
inline QuestsResult decode_quests(const fw::AccountResult& result){
    return decode_progression_profile<fw::gw::quests::Profile>(result,49160,fw::gw::quests::decode);
}
}
