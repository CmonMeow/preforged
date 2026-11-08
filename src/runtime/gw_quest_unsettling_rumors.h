#pragma once
#include "gw_quest_restore.h"
#include "gw_effect_registry.h"
#include <string_view>

namespace fw::gw::quests::unsettling_rumors {
inline constexpr std::uint32_t id=62;
inline constexpr unsigned experience=250;
inline constexpr std::string_view wiki_sha256="8ba56c0b300e2adcb51e7186421813e3b299490996c48d397c1d885fb6bfcc7e";
// These are server-owned ordered objective keys, never client agent IDs.
inline constexpr std::uint64_t meerak_credit=1,armin_credit=2;
enum class Action {accept,talk_meerak,talk_armin,reward};
struct Interaction {
    skills::effects::Scope selected,current;
    std::string_view captured_name;
    unsigned map=0;
    bool reachable=false;
};
struct Eligibility {bool prophecies=false,presearing=false;bool primary_test_completed=false;};
inline bool eligible(const Eligibility& e){return e.prophecies&&e.presearing&&e.primary_test_completed;}
inline bool matches(const Interaction& npc,unsigned map,std::string_view name){
    return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&
        npc.selected==npc.current&&npc.map==map&&npc.captured_name==name&&npc.reachable;
}
inline bool valid_progress(const Progress& p){
    if(!p.cycle||p.timed)return false;
    const auto& credits=p.objective_credits;
    if(p.stage==Stage::active)return credits.empty()||credits==std::set<std::uint64_t>{meerak_credit};
    if(p.stage==Stage::completed||p.stage==Stage::rewarded)
        return credits==std::set<std::uint64_t>{meerak_credit,armin_credit};
    return p.stage==Stage::abandoned;
}
// A proposal only: callers persist it with their existing account revision
// transaction. For reward that same transaction MUST also grant the captured
// Tall Shield and 250 XP. A verified item plan is required, not an XP-only fallback.
// Current scopes and reachability must be read under the world lock. Recheck
// them if a request is queued. Native dialogue choice IDs are not inferred here.
inline std::optional<Profile> plan(const Profile& current,Action action,
    std::uint32_t cycle,const Eligibility& e,const Interaction& npc,bool verified_reward_item_plan=false){
    validate(current);
    if(!eligible(e))return {};
    if(action==Action::accept){
        if(!matches(npc,146,"Devona"))return {};
        auto proposed=quests::accept(current,id);
        if(!proposed||proposed->records.at(id).cycle!=cycle)return {};
        return proposed;
    }
    const auto found=current.records.find(id);
    if(found==current.records.end()||found->second.cycle!=cycle||!valid_progress(found->second))return {};
    const auto& progress=found->second;
    if(action==Action::talk_meerak){
        if(!matches(npc,164,"Meerak the Scribe")||!progress.objective_credits.empty())return {};
        return credit_objective(current,id,cycle,meerak_credit,2);
    }
    if(action==Action::talk_armin){
        if(!matches(npc,148,"Armin Saberlin")||progress.objective_credits!=std::set<std::uint64_t>{meerak_credit})return {};
        return credit_objective(current,id,cycle,armin_credit,2);
    }
    if(action==Action::reward&&verified_reward_item_plan&&matches(npc,148,"Armin Saberlin"))
        return advance(current,id,cycle,Stage::completed,Stage::rewarded);
    return {};
}
// Storage rejects skipped objectives and direct reward writes; NPC authority is
// checked by the controller before this revision-checked persistence boundary.
inline bool valid_transition(const Profile& previous,const Profile& next,bool primary_test_completed){
    validate(previous);validate(next);
    const auto before=previous.records.find(id),after=next.records.find(id);
    if(before==previous.records.end()&&after==next.records.end())return true;
    if(before!=previous.records.end()&&after!=next.records.end()&&before->second==after->second)return true;
    if(after==next.records.end()||!valid_progress(after->second))return false;
    std::optional<Profile> proposed;
    if(after->second.stage==Stage::active&&after->second.objective_credits.empty()&&primary_test_completed)
        proposed=quests::accept(previous,id);
    if(before!=previous.records.end()&&valid_progress(before->second)){
        const auto cycle=before->second.cycle;
        if(after->second.stage==Stage::abandoned)proposed=quests::abandon(previous,id,cycle);
        else if(before->second.stage==Stage::active&&primary_test_completed){
            if(before->second.objective_credits.empty())proposed=credit_objective(previous,id,cycle,meerak_credit,2);
            else if(before->second.objective_credits==std::set<std::uint64_t>{meerak_credit})
                proposed=credit_objective(previous,id,cycle,armin_credit,2);
        }
    }
    return proposed&&*proposed==next;
}
// Checkpoint-1815 snapshots19062/23809/24751/24789 and packet564872.
// Armin's captured turn-in location is reused for the preceding delivery leg.
// Deep-copy variants: a session binding must never mutate the shared catalog.
inline Catalog bind_markers(const Catalog& source,unsigned current_map){
    auto result=source;const auto found=result.find(id);if(found==result.end())return result;
    auto bind=[&](Content& content,bool delivery){
        auto& entry=content.entry;entry.map_to=delivery?148:164;
        entry.x=entry.y=std::bit_cast<float>(0x7f800000u);entry.plane=0;
        if(!delivery){
            if(current_map==146){entry.x=-11308;entry.y=-6195;}
            if(current_map==147){entry.x=-11712;entry.y=-17471;}
            if(current_map==164){entry.x=-12342;entry.y=-6538;}
        }else{
            if(current_map==164){entry.x=-11215;entry.y=-6200;}
            if(current_map==148){entry.x=12035;entry.y=3116;}
        }
    };
    auto& content=found->second;bind(content,false);
    for(auto& [count,variant]:content.objective_variants){
        if(!variant)continue;variant=std::make_shared<Content>(*variant);bind(*variant,true);
    }
    if(content.completed_variant){content.completed_variant=std::make_shared<Content>(*content.completed_variant);bind(*content.completed_variant,true);}
    return result;
}
}
