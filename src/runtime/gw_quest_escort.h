#pragma once
#include "gw_effect_registry.h"
#include <optional>
#include <set>
#include <span>
namespace fw::gw::quests {
// Monk Test capture marks completion after three blocker deaths. Wiki also
// requires escorting Gwen to Ciglo for reward. World code must verify proximity,
// death and arrival; this controller accepts no client completion assertions.
class EscortObjective {
public:
    using Scope=skills::effects::Scope;
    enum class State {searching,blocked,following,arrived,failed};
private:
    Scope owner_,escort_,recipient_;unsigned cycle_=0;bool discovered_=false;
    std::set<Scope> remaining_;State state_=State::searching;
    static bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
    bool owns(Scope owner,unsigned cycle)const{return owner==owner_&&cycle==cycle_;}
    void refresh(){state_=!discovered_?State::searching:remaining_.empty()?State::following:State::blocked;}
public:
    static std::optional<EscortObjective> create(Scope owner,Scope escort,Scope recipient,unsigned cycle,std::span<const Scope> blockers){
        if(!valid(owner)||!valid(escort)||!valid(recipient)||owner.instance!=escort.instance||owner.instance!=recipient.instance||
           owner==escort||owner==recipient||escort==recipient||!cycle||blockers.size()!=3)return {};
        EscortObjective result;result.owner_=owner;result.escort_=escort;result.recipient_=recipient;result.cycle_=cycle;
        for(auto blocker:blockers)if(!valid(blocker)||blocker.instance!=owner.instance||blocker==owner||blocker==escort||blocker==recipient||!result.remaining_.insert(blocker).second)return {};
        return result;
    }
    bool discover(Scope owner,unsigned cycle,Scope escort){
        if(!owns(owner,cycle)||escort!=escort_||discovered_||state_==State::failed)return false;
        discovered_=true;refresh();return true;
    }
    // Trusted durable completed state resumes the escort, not reward-ready
    // arrival. Old blocker lifetimes must not be recreated on reconnect.
    static std::optional<EscortObjective> restore_following(Scope owner,Scope escort,Scope recipient,unsigned cycle){
        if(!valid(owner)||!valid(escort)||!valid(recipient)||owner.instance!=escort.instance||owner.instance!=recipient.instance||
           owner==escort||owner==recipient||escort==recipient||!cycle)return {};
        EscortObjective result;result.owner_=owner;result.escort_=escort;result.recipient_=recipient;result.cycle_=cycle;
        result.discovered_=true;result.state_=State::following;return result;
    }
    bool death(Scope owner,unsigned cycle,Scope blocker){
        // The world dispatches every NPC death here. Losing an escort
        // participant must also revoke a pending arrival reward.
        if(invalidated(owner,cycle,blocker))return true;
        if(!owns(owner,cycle)||state_==State::failed||state_==State::arrived||!remaining_.erase(blocker))return false;
        refresh();return true;
    }
    bool arrive(Scope owner,unsigned cycle,Scope escort,Scope recipient){
        if(!owns(owner,cycle)||escort!=escort_||recipient!=recipient_||state_!=State::following)return false;
        state_=State::arrived;return true;
    }
    bool invalidated(Scope owner,unsigned cycle,Scope actor){
        if(!owns(owner,cycle)||state_==State::failed||(actor!=owner_&&actor!=escort_&&actor!=recipient_))return false;
        state_=State::failed;return true;
    }
    bool objective_complete()const{return state_==State::following||state_==State::arrived;}
    bool reward_ready()const{return state_==State::arrived;}
    State state()const{return state_;}
    unsigned remaining()const{return static_cast<unsigned>(remaining_.size());}
    unsigned cycle()const{return cycle_;}
    Scope escort()const{return escort_;}
    Scope recipient()const{return recipient_;}
};
}
