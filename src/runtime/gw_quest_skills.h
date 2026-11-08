#pragma once
#include "gw_character_skills.h"
#include "gw_presearing_quests.h"
#include "gw_quest_state.h"
#include "gw_quest_second_profession.h"
#include "gw_quest_message_from_friend.h"
#include "gw_quest_path_to_glory.h"
#include "gw_quest_ranger_companion.h"
#include <set>

namespace fw::gw::quests {
// Quest acceptance adds newly taught usable skills to vacant slots. Captured
// Experiment73 responses at100074984 pair each learned count with a slot write.
// Never replace a player's existing build or reinsert a skill already learned.
inline skills::Profile teach(const skills::Profile& current,
    std::span<const std::uint32_t> grants,bool fill_vacancies){
    auto proposed=current;std::set<std::uint32_t> unique;
    for(auto id:grants){
        const auto* skill=skills::find_skill(id);
        if(!id||!skill||!unique.insert(id).second)
            throw std::invalid_argument("invalid acceptance skill");
        if(proposed.learned.contains(id))continue;
        proposed.learned.emplace(id,1);
        if(!fill_vacancies||(skill->profession&&skill->profession!=proposed.primary&&
            skill->profession!=proposed.secondary&&!proposed.trial_skills.contains(id)))continue;
        for(auto& slot:proposed.bar)if(!slot.skill){slot={id,0};break;}
    }
    skills::validate_profile(proposed);return proposed;
}
// Produce a single candidate for the caller's quest/skills transaction. This
// does not commit a secondary or grant profession-switch privileges.
inline skills::Profile teach_trial(const skills::Profile& current,std::uint32_t quest){
    skills::validate_profile(current);
    const auto* rule=presearing::find(quest);
    if(!rule||rule->primary_only||current.secondary||current.primary==rule->profession)
        throw std::invalid_argument("quest is not an unchosen-secondary trial");
    auto proposed=current;std::vector<std::uint32_t> grants;
    for(auto id:rule->acceptance_skills)if(id){
        proposed.learned.try_emplace(id,1);proposed.trial_skills.insert(id);grants.push_back(id);
    }
    // Newly authorized trials fill vacancies even on reacceptance. A skill
    // removed while its trial remains authorized is not reinserted.
    for(auto id:grants)if(!current.trial_skills.contains(id)&&
        std::none_of(proposed.bar.begin(),proposed.bar.end(),[&](auto slot){return slot.skill==id;}))
        for(auto& slot:proposed.bar)if(!slot.skill){slot={id,0};break;}
    skills::validate_profile(proposed);return proposed;
}
inline skills::Profile teach_acceptance(const skills::Profile& current,std::uint32_t quest,
    std::span<const std::uint32_t> grants,bool fill,bool trial){
    if(!trial)return teach(current,grants,fill);
    const auto* rule=presearing::find(quest);std::vector<std::uint32_t> reviewed;
    if(rule)for(auto id:rule->acceptance_skills)if(id)reviewed.push_back(id);
    if(!fill||reviewed.size()!=grants.size()||!std::equal(reviewed.begin(),reviewed.end(),grants.begin()))
        throw std::invalid_argument("trial requires exact reviewed quest grants and slots");
    return teach_trial(current,quest);
}
// Private trial cancellation policy: retain learning, revoke authorization no
// longer backed by a non-abandoned trial quest, and clear its unusable slots.
// Rewarded trials remain available until final profession selection.
inline skills::Profile prune_trials(const skills::Profile& current,const Profile& quests){
    skills::validate_profile(current);validate(quests);auto proposed=current;
    std::set<std::uint32_t> retained;
    for(const auto& [id,progress]:quests.records){const auto* rule=presearing::find(id);
        if(!rule||rule->primary_only||rule->profession==current.primary||progress.stage==Stage::abandoned)continue;
        for(auto skill:rule->acceptance_skills)if(current.trial_skills.contains(skill))retained.insert(skill);}
    proposed.trial_skills=std::move(retained);
    for(auto& slot:proposed.bar){const auto* row=skills::find_skill(slot.skill);
        if(row&&row->profession&&row->profession!=proposed.primary&&row->profession!=proposed.secondary&&
            current.trial_skills.contains(slot.skill)&&!proposed.trial_skills.contains(slot.skill))slot={};}
    skills::validate_profile(proposed);return proposed;
}
// Reviewed training quests unlock eligibility, not the selected secondary.
// Final selection remains the independently decoded profession request.
// Local A_Second_Profession.html names these training quests. Quest76 now
// has persisted post-acceptance adoption;72/74 remain uninstalled.
inline bool reviewed_trial_unlock_quest(std::uint32_t quest){return quest==71||quest==73||quest==75||quest==76;}
inline skills::Profile unlock_trial(const skills::Profile& current,std::uint32_t quest){
    skills::validate_profile(current);const auto* rule=presearing::find(quest);
    if(!reviewed_trial_unlock_quest(quest)||!rule||current.secondary||current.primary==rule->profession)
        throw std::invalid_argument("quest cannot unlock trial profession");
    for(auto id:rule->acceptance_skills)if(id&&(!current.trial_skills.contains(id)||!current.learned.contains(id)))
        throw std::invalid_argument("profession unlock requires reviewed trial grants");
    auto proposed=current;proposed.profession_mask|=1u<<rule->profession;
    skills::validate_profile(proposed);return proposed;
}
}
