#pragma once
#include "gw_quest_state.h"
#include "gw_effect_registry.h"
#include <string_view>

namespace fw::gw::quests::new_elementalist_trainer {
inline constexpr std::uint32_t id=65;
inline constexpr unsigned experience=250;
struct Eligibility {bool prophecies=false,presearing=false;unsigned primary=0;};
struct Interaction {
    skills::effects::Scope selected,current;
    std::string_view name;
    unsigned map=0;
    bool reachable=false;
};
enum class Action {accept,talk_aziure,reward};
// Wiki/capture join: data/gw/quests/new-elementalist-trainer-evidence.json.
// Primary-only eligibility does not make Haversdan a retiring starter trainer.
inline bool eligible(const Profile& current,const Eligibility& e){
    if(!e.prophecies||!e.presearing||e.primary!=6)return false;
    const auto prerequisite=current.records.find(54);
    return prerequisite!=current.records.end()&&prerequisite->second.stage==Stage::rewarded;
}
inline bool matches(const Interaction& npc,unsigned map,std::string_view name){
    return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&
        npc.selected==npc.current&&npc.map==map&&npc.name==name&&npc.reachable;
}
// Pure proposal: completion is retryable separately from the existing atomic
// completed->rewarded/250 XP transaction. No skill grant or profession unlock.
inline std::optional<Profile> plan(const Profile& current,Action action,std::uint32_t cycle,
    const Eligibility& e,const Interaction& npc){
    validate(current);
    if(!eligible(current,e))return {};
    if(action==Action::accept){
        if(!matches(npc,146,"Haversdan"))return {};
        auto proposed=quests::accept(current,id);
        if(!proposed||proposed->records.at(id).cycle!=cycle)return {};
        return proposed;
    }
    if(!matches(npc,161,"Elementalist Aziure"))return {};
    const auto found=current.records.find(id);
    if(found==current.records.end()||!found->second.objective_credits.empty()||found->second.timed)return {};
    if(action==Action::talk_aziure)return advance(current,id,cycle,Stage::active,Stage::completed);
    if(action==Action::reward)return advance(current,id,cycle,Stage::completed,Stage::rewarded);
    return {};
}
}
