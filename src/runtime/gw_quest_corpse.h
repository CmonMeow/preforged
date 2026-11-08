#pragma once
#include "gw_effect_registry.h"
#include <optional>
#include <set>
#include <span>
namespace fw::gw::quests {
// Host-only staged objective. World code verifies death, proximity, corpse
// reservation and successful NPC publication; none are client assertions.
// Quest82 capture establishes publication before completion, not a player
// Animate Bone Horror cast time or a reusable retail dialog identifier.
class CorpseObjective {
public:
    using Scope=skills::effects::Scope;
    enum class State {waiting_death,waiting_corpse,waiting_summon,completed,failed};
private:
    Scope owner_,trainer_;std::uint32_t cycle_=0;
    std::set<Scope> candidates_;std::optional<Scope> corpse_,summon_;
    State state_=State::waiting_death;
    bool owns(Scope owner,std::uint32_t cycle)const{return owner==owner_&&cycle==cycle_;}
    static bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
public:
    static std::optional<CorpseObjective> create(Scope owner,Scope trainer,std::uint32_t cycle,std::span<const Scope> candidates){
        if(!valid(owner)||!valid(trainer)||owner.instance!=trainer.instance||owner==trainer||!cycle||candidates.empty()||candidates.size()>64)return {};
        CorpseObjective result;result.owner_=owner;result.trainer_=trainer;result.cycle_=cycle;
        for(auto actor:candidates)if(!valid(actor)||actor.instance!=owner.instance||actor==owner||actor==trainer||!result.candidates_.insert(actor).second)return {};
        return result;
    }
    bool death(Scope owner,std::uint32_t cycle,Scope actor){
        if(!owns(owner,cycle)||state_!=State::waiting_death||!candidates_.contains(actor))return false;
        corpse_=actor;state_=State::waiting_corpse;return true;
    }
    // Trusted durable completed state: transient corpse/summon lifetimes from
    // the previous instance must not be replayed or invented on reconnect.
    static std::optional<CorpseObjective> restore_completed(Scope owner,Scope trainer,std::uint32_t cycle){
        if(!valid(owner)||!valid(trainer)||owner.instance!=trainer.instance||owner==trainer||!cycle)return {};
        CorpseObjective result;result.owner_=owner;result.trainer_=trainer;result.cycle_=cycle;result.state_=State::completed;
        return result;
    }
    bool corpse_claimed(Scope owner,std::uint32_t cycle,Scope trainer,Scope corpse){
        if(!owns(owner,cycle)||trainer!=trainer_||state_!=State::waiting_corpse||corpse_!=corpse)return false;
        state_=State::waiting_summon;return true;
    }
    bool summon_published(Scope owner,std::uint32_t cycle,Scope trainer,Scope summon){
        if(!owns(owner,cycle)||trainer!=trainer_||state_!=State::waiting_summon||!valid(summon)||summon.instance!=owner_.instance||
           summon==owner_||summon==trainer_||candidates_.contains(summon))return false;
        summon_=summon;state_=State::completed;return true;
    }
    bool invalidated(Scope owner,std::uint32_t cycle,Scope actor){
        if(!owns(owner,cycle)||state_==State::completed||state_==State::failed)return false;
        if(actor!=owner_&&actor!=trainer_&&!(state_==State::waiting_corpse&&corpse_==actor))return false;
        state_=State::failed;return true;
    }
    State state()const{return state_;}
    std::optional<Scope> corpse()const{return corpse_;}
    std::optional<Scope> summon()const{return summon_;}
    Scope trainer()const{return trainer_;}
    std::uint32_t cycle()const{return cycle_;}
};
}
