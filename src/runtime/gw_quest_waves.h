#pragma once
#include "gw_effect_registry.h"
#include <set>
#include <optional>
#include <span>
namespace fw::gw::quests {
// Wave progression is independent of wire actor IDs and durable quest state.
// Spawn publication must succeed before bind; completion still needs quest CAS.
class WaveObjective {
public:
    using Scope=skills::effects::Scope;
    enum class State {ready,fighting,completed,failed};
private:
    Scope owner_,guardian_;std::uint32_t quest_=0,cycle_=0;
    unsigned waves_=0,size_=0,index_=0;State state_=State::ready;
    std::set<Scope> remaining_,used_;
public:
    static std::optional<WaveObjective> create(Scope owner,Scope guardian,
        std::uint32_t quest,std::uint32_t cycle,unsigned waves,unsigned wave_size){
        if(!owner.instance||!owner.actor||!owner.generation||!guardian.actor||!guardian.generation||
           guardian.instance!=owner.instance||guardian==owner||!quest||!cycle||
           !waves||waves>16||!wave_size||wave_size>64)return {};
        WaveObjective result;result.owner_=owner;result.guardian_=guardian;result.quest_=quest;
        result.cycle_=cycle;result.waves_=waves;result.size_=wave_size;return result;
    }
    bool bind(Scope owner,std::uint32_t cycle,std::span<const Scope> actors){
        if(owner!=owner_||cycle!=cycle_||state_!=State::ready||actors.size()!=size_)return false;
        std::set<Scope> staged;
        for(auto actor:actors)if(!actor.actor||!actor.generation||actor.instance!=owner_.instance||
            actor==owner_||actor==guardian_||used_.contains(actor)||!staged.insert(actor).second)return false;
        remaining_=staged;used_.insert(staged.begin(),staged.end());state_=State::fighting;return true;
    }
    // Only a host-loaded durable completed quest may use this restoration path.
    static std::optional<WaveObjective> restore_completed(Scope owner,Scope guardian,
        std::uint32_t quest,std::uint32_t cycle,unsigned waves,unsigned size){
        auto restored=create(owner,guardian,quest,cycle,waves,size);
        if(restored){restored->index_=waves;restored->state_=State::completed;}
        return restored;
    }
    bool death(Scope owner,std::uint32_t cycle,Scope actor){
        if(owner!=owner_||cycle!=cycle_||state_==State::failed)return false;
        // Guardian loss also invalidates a pending final reward.
        if(actor==guardian_){remaining_.clear();state_=State::failed;return true;}
        if(state_!=State::fighting||!remaining_.erase(actor))return false;
        if(remaining_.empty()){++index_;state_=index_==waves_?State::completed:State::ready;}
        return true;
    }
    State state()const{return state_;}
    unsigned wave()const{return index_;}
    unsigned wave_count()const{return waves_;}
    unsigned wave_size()const{return size_;}
    std::uint32_t quest()const{return quest_;}
    std::uint32_t cycle()const{return cycle_;}
    Scope guardian()const{return guardian_;}
};
}
