#pragma once
#include "gw_quest_state.h"
#include "gw_effect_registry.h"
#include <array>

namespace fw::gw::quests::monk_mission {
inline constexpr unsigned id=74,experience=250;
inline constexpr std::array<unsigned,2> acceptance_skills{296,281};
using Scope=skills::effects::Scope;
struct Eligibility {bool prophecies=false,presearing=false;unsigned primary=0,secondary=0;};
inline bool eligible(const Profile& profile,const Eligibility& e){
    if(!e.prophecies||!e.presearing||e.primary<1||e.primary>6||
       (e.primary!=3&&e.secondary!=3&&e.secondary!=0))return false;
    constexpr std::array<unsigned,6> tests{85,86,84,82,81,83};
    const auto found=profile.records.find(tests[e.primary-1]);
    return found!=profile.records.end()&&found->second.stage==Stage::rewarded;
}
// Trusted world events only. Observed coordinates in monk-mission-evidence.json
// are not movement instructions. Integration must verify map145, proximity,
// actor liveness and authored route milestones before delivering these events.
class Escort {
public:
    enum class Phase {waiting,outbound,returning,arrived,failed};
private:
    Scope owner_,paulus_;unsigned cycle_=0;Phase phase_=Phase::waiting;
    static bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
    bool matches(Scope owner,unsigned cycle,Scope paulus)const{
        return owner==owner_&&cycle==cycle_&&paulus==paulus_;
    }
public:
    static std::optional<Escort> create(Scope owner,Scope paulus,unsigned cycle){
        if(!valid(owner)||!valid(paulus)||owner.instance!=paulus.instance||owner.actor==paulus.actor||!cycle)return {};
        Escort result;result.owner_=owner;result.paulus_=paulus;result.cycle_=cycle;return result;
    }
    bool discover(Scope owner,unsigned cycle,Scope paulus){
        if(!matches(owner,cycle,paulus)||phase_!=Phase::waiting)return false;
        phase_=Phase::outbound;return true;
    }
    bool artifact_retrieved(Scope owner,unsigned cycle,Scope paulus){
        if(!matches(owner,cycle,paulus)||phase_!=Phase::outbound)return false;
        phase_=Phase::returning;return true;
    }
    bool returned(Scope owner,unsigned cycle,Scope paulus){
        if(!matches(owner,cycle,paulus)||phase_!=Phase::returning)return false;
        phase_=Phase::arrived;return true;
    }
    bool invalidated(Scope actor){
        if(phase_==Phase::failed||(actor!=owner_&&actor!=paulus_))return false;
        phase_=Phase::failed;return true;
    }
    bool may_move(Scope owner,unsigned cycle,Scope paulus,bool owner_near,bool paulus_alive)const{
        return matches(owner,cycle,paulus)&&owner_near&&paulus_alive&&
            (phase_==Phase::outbound||phase_==Phase::returning);
    }
    std::optional<Profile> complete(const Profile& current,Scope owner,unsigned cycle,Scope paulus)const{
        validate(current);
        if(!matches(owner,cycle,paulus)||phase_!=Phase::arrived)return {};
        const auto found=current.records.find(id);
        if(found==current.records.end()||found->second.timed||!found->second.objective_credits.empty())return {};
        return advance(current,id,cycle,Stage::active,Stage::completed);
    }
    Phase phase()const{return phase_;}
};
// Acceptance/skill grant and reward/XP must use account transactions. No menu
// rule is installed until world escort integration can actually finish quest74.
}
