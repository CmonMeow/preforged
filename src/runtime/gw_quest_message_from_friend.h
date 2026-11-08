#pragma once
#include "gw_quest_restore.h"
#include "gw_effect_registry.h"
#include <array>
#include <string_view>
namespace fw::gw::quests::message_from_friend {
inline constexpr unsigned id=80,experience=100,gold=10;
struct Eligibility {bool prophecies=false,presearing=false;};
struct Interaction {skills::effects::Scope selected,current;std::string_view name;unsigned map=0;bool reachable=false;};
inline bool eligible(const Eligibility& e){return e.prophecies&&e.presearing;}
inline bool admission_open(const Profile& current,const Eligibility& e){
    if(!eligible(e))return false;
    // Quest.cs names35 and217..222 War Preparations. Do not infer completion
    // from another quest's absence or require a primary/secondary profession.
    constexpr std::array<unsigned,7> preparations{35,217,218,219,220,221,222};
    for(auto quest:preparations){const auto found=current.records.find(quest);
        if(found!=current.records.end()&&(found->second.stage==Stage::completed||found->second.stage==Stage::rewarded))return false;}
    return true;
}
inline bool matches(const Interaction& npc,std::string_view name){
    return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&npc.selected==npc.current&&
        npc.name==name&&npc.map==148&&npc.reachable;
}
inline bool valid_progress(const Progress& progress){return progress.cycle&&progress.objective_credits.empty()&&!progress.timed;}
inline std::optional<Profile> accept(const Profile& current,unsigned cycle,const Eligibility& e,const Interaction& npc){
    validate(current);if(!admission_open(current,e)||!matches(npc,"Town Crier"))return {};
    auto proposed=quests::accept(current,id);
    if(!proposed||proposed->records.at(id).cycle!=cycle)return {};
    return proposed;
}
// Captured marker/details refresh, XP100, gold10 and removal share a Tydus
// burst. PRIVATE storage policy closes both transitions in one transaction;
// absent completed snapshots establish no retail durability/trigger policy.
inline std::optional<Profile> reward_progress(const Profile& current,unsigned cycle){
    validate(current);const auto found=current.records.find(id);
    if(found==current.records.end()||!valid_progress(found->second))return {};
    const auto completed=advance(current,id,cycle,Stage::active,Stage::completed);
    if(!completed)return {};
    return advance(*completed,id,cycle,Stage::completed,Stage::rewarded);
}
inline std::optional<Profile> reward(const Profile& current,unsigned cycle,const Eligibility& e,const Interaction& npc){
    if(!eligible(e)||!matches(npc,"Sir Tydus"))return {};
    return reward_progress(current,cycle);
}
inline bool valid_transition(const Profile& previous,const Profile& next){
    validate(previous);validate(next);
    const auto before=previous.records.find(id),after=next.records.find(id);
    if(before==previous.records.end()&&after==next.records.end())return true;
    if(before!=previous.records.end()&&after!=next.records.end()&&before->second==after->second)return true;
    if(after==next.records.end()||!valid_progress(after->second))return false;
    std::optional<Profile> proposed;
    if(after->second.stage==Stage::active&&admission_open(previous,{true,true}))proposed=quests::accept(previous,id);
    if(before!=previous.records.end()&&after->second.stage==Stage::abandoned)
        proposed=quests::abandon(previous,id,before->second.cycle);
    return proposed&&*proposed==next;
}
inline Catalog bind_markers(const Catalog& source,unsigned current_map){
    auto result=source;const auto found=result.find(id);if(found==result.end())return result;
    auto& entry=found->second.entry;entry.map_to=148;
    entry.x=entry.y=std::bit_cast<float>(0x7f800000u);entry.plane=0;
    if(current_map==148){entry.x=11715;entry.y=3517;entry.plane=26;}
    if(current_map==146){entry.x=7559;entry.y=5596;}
    return result;
}
}
