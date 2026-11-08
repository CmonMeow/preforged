#pragma once
#include "gw_effect_registry.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <vector>
namespace fw::gw::shrine {
using Scope=skills::effects::Scope;using Time=std::uint64_t;
inline constexpr Time wipe_delay=10000; // Reviewed local Resurrection.html39.
struct Party {
    std::uint64_t instance=0,epoch=0;unsigned id=0;
    bool operator==(const Party&)const=default;
};
struct Member {Scope scope;std::optional<bool> alive;};
struct Snapshot {
    Party party;std::vector<Member> players;Time at=0;
    // Diagnostic only. Pets never prevent an all-player-dead party wipe.
    unsigned living_pets=0;
};
struct Attempt {
    std::uint64_t serial=0;Party party;std::vector<Scope> members;Time began=0,due=0;
    bool operator==(const Attempt&)const=default;
};
enum class Code {invalid,alive,unknown_liveness,roster_changed,armed,waiting,ready,already_claimed};
struct Observation {Code code=Code::invalid;std::optional<Attempt> attempt;};
// Pure scheduling authority, not a shrine selector or resurrection producer.
// World supplies exact current player-party scopes and monotonic event times.
// A roster change MUST advance the party epoch. Same-epoch roster changes
// invalidate the old timer and fail closed until a fresh epoch is supplied.
class WipeScheduler {
    struct State {std::uint64_t epoch=0;Time clock=0;std::vector<Scope> members;
        bool blocked=false,claimed=false;std::optional<Attempt> attempt;};
    std::map<std::pair<std::uint64_t,unsigned>,State> states_;
    std::uint64_t serial_=0;
    static std::optional<std::vector<Scope>> roster(const Snapshot& facts){
        if(!facts.party.instance||!facts.party.id||!facts.party.epoch)return {};
        std::vector<Scope> members;std::set<unsigned> actors;
        for(const auto& member:facts.players){const auto s=member.scope;
            if(s.instance!=facts.party.instance||!s.actor||s.actor>65535||!s.generation||!actors.insert(s.actor).second)return {};
            members.push_back(s);
        }
        std::sort(members.begin(),members.end());return members;
    }
public:
    Observation observe(const Snapshot& facts){
        const auto members=roster(facts);if(!members)return {};
        const auto key=std::pair{facts.party.instance,facts.party.id};auto& state=states_[key];
        if((state.epoch&&facts.party.epoch<state.epoch)||(state.epoch&&facts.at<state.clock))return {};
        state.clock=facts.at;
        if(facts.party.epoch>state.epoch){state.epoch=facts.party.epoch;state.members=*members;
            state.blocked=state.claimed=false;state.attempt.reset();}
        else{
            // Report the first same-epoch mutation once. Subsequent snapshots
            // cannot reinterpret that invalid epoch or revive its old timer.
            if(state.blocked)return {};
            if(state.members!=*members){state.attempt.reset();state.blocked=true;return {Code::roster_changed,{}};}
        }
        if(members->empty()){state.attempt.reset();state.blocked=true;return {Code::roster_changed,{}};}
        if(std::any_of(facts.players.begin(),facts.players.end(),[](const auto& p){return p.alive==std::optional(true);})){state.attempt.reset();state.claimed=false;return {Code::alive,{}};}
        if(std::any_of(facts.players.begin(),facts.players.end(),[](const auto& p){return !p.alive.has_value();})){state.attempt.reset();return {Code::unknown_liveness,{}};}
        if(state.claimed)return {Code::already_claimed,{}};
        if(!state.attempt){
            if(serial_==UINT64_MAX||facts.at>UINT64_MAX-wipe_delay)return {};
            state.attempt=Attempt{++serial_,facts.party,*members,facts.at,facts.at+wipe_delay};
            return {Code::armed,state.attempt};
        }
        return {facts.at>=state.attempt->due?Code::ready:Code::waiting,state.attempt};
    }
    // Call only AFTER World prepares the complete no-fail recovery proposal,
    // including an activated reviewed site and all player/pet destinations.
    // Current all-dead facts are revalidated; a replay cannot consume a new wipe.
    bool claim(const Attempt& receipt,const Snapshot& current){
        if(current.party!=receipt.party)return false;
        const auto key=std::pair{receipt.party.instance,receipt.party.id};const auto found=states_.find(key);
        if(found==states_.end()||found->second.attempt!=std::optional(receipt))return false;
        const auto observation=observe(current);
        if(observation.code!=Code::ready||observation.attempt!=std::optional(receipt))return false;
        auto& state=states_.at(key);state.attempt.reset();state.claimed=true;return true;
    }
    std::optional<Attempt> pending(Party party)const{
        const auto found=states_.find({party.instance,party.id});
        return found!=states_.end()&&found->second.epoch==party.epoch&&!found->second.blocked?found->second.attempt:std::nullopt;
    }
    bool retire(Party party){
        const auto found=states_.find({party.instance,party.id});
        if(found==states_.end()||found->second.epoch!=party.epoch)return false;
        found->second.attempt.reset();found->second.blocked=true;return true;
    }
};
}
