#pragma once
#include "gw_quest_state.h"
#include "gw_effect_registry.h"
#include <string_view>

namespace fw::gw::quests::further_adventures {
inline constexpr std::uint32_t id=54;
inline constexpr unsigned experience=250;
// All six professions are eligible. Local Further_Adventures.html and the
// captured quest54/Devona reward burst are joined in further-adventures-evidence.json.
struct Eligibility {bool prophecies=false,presearing=false;unsigned primary=0;};
struct Interaction {
    skills::effects::Scope selected,current;
    std::string_view name;
    unsigned map=0;
    bool reachable=false;
};
enum class Action {accept,talk_devona,reward};
inline bool eligible(const Profile& current,const Eligibility& e){
    constexpr std::uint32_t tests[]{0,85,86,84,82,81,83};
    if(!e.prophecies||!e.presearing||e.primary<1||e.primary>6)return false;
    const auto found=current.records.find(tests[e.primary]);
    return found!=current.records.end()&&found->second.stage==Stage::rewarded;
}
inline bool matches(const Interaction& npc,std::string_view name){
    return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&
        npc.selected==npc.current&&npc.map==146&&npc.name==name&&npc.reachable;
}
// A pure proposal. Completing the conversation persists independently, so a
// failed reward can be retried. The existing reward transaction atomically
// persists rewarded state and 250 XP, and rejects stale/duplicate requests.
inline std::optional<Profile> plan(const Profile& current,Action action,std::uint32_t cycle,
    const Eligibility& e,const Interaction& npc){
    validate(current);
    if(!eligible(current,e))return {};
    if(action==Action::accept){
        if(!matches(npc,"Haversdan"))return {};
        auto proposed=quests::accept(current,id);
        if(!proposed||proposed->records.at(id).cycle!=cycle)return {};
        return proposed;
    }
    if(!matches(npc,"Devona"))return {};
    const auto found=current.records.find(id);
    if(found==current.records.end()||!found->second.objective_credits.empty()||found->second.timed)return {};
    if(action==Action::talk_devona)return advance(current,id,cycle,Stage::active,Stage::completed);
    if(action==Action::reward)return advance(current,id,cycle,Stage::completed,Stage::rewarded);
    return {};
}
}
