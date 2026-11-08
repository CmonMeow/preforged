#pragma once
#include "gw_quest_restore.h"
#include "gw_effect_registry.h"
#include <array>
#include <string_view>

namespace fw::gw::quests::second_profession {
inline constexpr unsigned id=79,experience=250;
struct Eligibility {bool prophecies=false,presearing=false;unsigned primary=0,secondary=0;};
struct Interaction {skills::effects::Scope selected,current;std::string_view name;unsigned map=0;bool reachable=false;};
inline constexpr std::array<unsigned,7> primary_tests{0,85,86,84,82,81,83};
inline constexpr std::array<unsigned,7> secondary_training{0,75,76,74,72,71,73};
inline bool rewarded(const Profile& current,unsigned quest){
    const auto found=current.records.find(quest);
    return found!=current.records.end()&&found->second.stage==Stage::rewarded;
}
inline bool eligible(const Profile& current,const Eligibility& e){
    return e.prophecies&&e.presearing&&e.primary>=1&&e.primary<=6&&
        e.secondary<=6&&e.secondary!=e.primary&&rewarded(current,primary_tests[e.primary]);
}
// A trial, an unlocked mask bit, a menu choice or an attempted/failed save is
// not a selected secondary. Callers load the durable skill profile; storage
// rechecks it inside its quest transaction before allowing completion/reward.
inline bool objective_ready(const Profile& current,const Eligibility& e){
    return eligible(current,e)&&e.secondary&&rewarded(current,secondary_training[e.secondary]);
}
inline bool matches(const Interaction& npc){
    return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&npc.selected==npc.current&&
        npc.name=="Armin Saberlin"&&npc.map==148&&npc.reachable;
}
inline bool valid_progress(const Progress& progress){return progress.cycle&&progress.objective_credits.empty()&&!progress.timed;}
enum class Action {accept,complete,reward};
inline std::optional<Profile> plan(const Profile& current,Action action,unsigned cycle,
    const Eligibility& e,const Interaction& npc={}){
    validate(current);if(!eligible(current,e))return {};
    if(action==Action::accept){
        if(!matches(npc))return {};
        auto proposed=quests::accept(current,id);
        if(!proposed||proposed->records.at(id).cycle!=cycle)return {};
        return proposed;
    }
    const auto found=current.records.find(id);
    if(found==current.records.end()||!valid_progress(found->second)||!objective_ready(current,e))return {};
    if(action==Action::complete)return advance(current,id,cycle,Stage::active,Stage::completed);
    if(action==Action::reward&&matches(npc))return advance(current,id,cycle,Stage::completed,Stage::rewarded);
    return {};
}
// Storage authority for a changed79 record. A generic SaveQuests cannot insert
// a completed/rewarded record, erase history, grant other progress alongside79,
// or complete from an uncommitted selected-secondary preview.
inline bool valid_transition(const Profile& previous,const Profile& next,const Eligibility& e){
    validate(previous);validate(next);
    const auto before=previous.records.find(id),after=next.records.find(id);
    if(before==previous.records.end()&&after==next.records.end())return true;
    if(before!=previous.records.end()&&after!=next.records.end()&&before->second==after->second)return true;
    if(after==next.records.end()||!valid_progress(after->second))return false;
    std::optional<Profile> proposed;
    if(after->second.stage==Stage::active&&eligible(previous,e))proposed=quests::accept(previous,id);
    if(before!=previous.records.end()){
        if(after->second.stage==Stage::abandoned)proposed=quests::abandon(previous,id,before->second.cycle);
        if(after->second.stage==Stage::completed)proposed=plan(previous,Action::complete,after->second.cycle,e);
    }
    return proposed&&*proposed==next;
}
// Capture-backed CURRENT-map routes to Armin, from the October9 W/E world.
// Missing routes retain the native unresolved sentinel; no portal is guessed.
inline void route_marker(Entry& entry,unsigned current_map){
    struct Marker {unsigned map;float x,y;};
    constexpr Marker markers[]={{148,12035,3116},{146,7559,5596},{160,22279,13121},
        {161,10796,19847},{164,-11215,-6200},{165,430,7598}};
    entry.map_to=148;entry.plane=0;
    entry.x=entry.y=std::bit_cast<float>(0x7f800000u);
    for(auto marker:markers)if(marker.map==current_map){entry.x=marker.x;entry.y=marker.y;return;}
}
inline Catalog bind_markers(const Catalog& source,unsigned current_map){
    auto result=source;const auto found=result.find(id);if(found==result.end())return result;
    route_marker(found->second.entry,current_map);
    if(found->second.completed_variant){
        found->second.completed_variant=std::make_shared<Content>(*found->second.completed_variant);
        route_marker(found->second.completed_variant->entry,current_map);
    }
    return result;
}
}
