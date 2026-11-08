#pragma once
#include "gw_quest_state.h"
#include "gw_effect_registry.h"
#include <string_view>

namespace fw::gw::quests::wayward_wizard {
inline constexpr std::uint32_t id=55;
inline constexpr unsigned experience=250;
inline constexpr std::string_view wiki_sha256="56987929fccd4253ca2e75ae41dff3e2db8329131ab5b8780cf96ad90db6f61d";
// These are server-owned ordered objective keys, never client agent IDs.
inline constexpr std::uint64_t ralena_credit=1,orion_credit=2;
enum class Action {accept,talk_ralena,talk_orion,reward};
struct Interaction {
    skills::effects::Scope selected,current;
    std::string_view captured_name;
    unsigned map=0;
    bool reachable=false;
};
struct Eligibility {bool prophecies=false,presearing=false;unsigned level=0;};
inline bool eligible(const Eligibility& e){return e.prophecies&&e.presearing&&e.level>=2;}
inline bool matches(const Interaction& npc,unsigned map,std::string_view name){
    return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&
        npc.selected==npc.current&&npc.map==map&&npc.captured_name==name&&npc.reachable;
}
inline bool valid_progress(const Progress& p){
    const auto& credits=p.objective_credits;
    if(p.stage==Stage::active)return credits.empty()||credits==std::set<std::uint64_t>{ralena_credit};
    if(p.stage==Stage::completed||p.stage==Stage::rewarded)
        return credits==std::set<std::uint64_t>{ralena_credit,orion_credit};
    return p.stage==Stage::abandoned;
}
// A proposal only: callers persist it with their existing account revision
// transaction. For reward that same transaction MUST also grant the captured
// Air Staff and 250 XP. A verified item plan is required, not an XP-only fallback.
// Current scopes and reachability must be read under the world lock. Recheck
// them if a request is queued. Native dialogue choice IDs are not inferred here.
inline std::optional<Profile> plan(const Profile& current,Action action,
    std::uint32_t cycle,const Eligibility& e,const Interaction& npc,bool verified_reward_item_plan=false){
    validate(current);
    if(!eligible(e))return {};
    if(action==Action::accept){
        if(!matches(npc,148,"Sandre Elek"))return {};
        auto proposed=quests::accept(current,id);
        if(!proposed||proposed->records.at(id).cycle!=cycle)return {};
        return proposed;
    }
    const auto found=current.records.find(id);
    if(found==current.records.end()||found->second.cycle!=cycle||!valid_progress(found->second))return {};
    const auto& progress=found->second;
    if(action==Action::talk_ralena){
        if(!matches(npc,165,"Ralena Stormbringer")||!progress.objective_credits.empty())return {};
        return credit_objective(current,id,cycle,ralena_credit,2);
    }
    if(action==Action::talk_orion){
        if(!matches(npc,161,"Orion Elek")||progress.objective_credits!=std::set<std::uint64_t>{ralena_credit})return {};
        return credit_objective(current,id,cycle,orion_credit,2);
    }
    if(action==Action::reward&&verified_reward_item_plan&&matches(npc,148,"Sandre Elek"))
        return advance(current,id,cycle,Stage::completed,Stage::rewarded);
    return {};
}
}
