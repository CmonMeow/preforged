#pragma once
#include "gw_signet_humility.h"
#include "gw_disable_reset.h"

namespace fw::gw::skills::effects {
class HumilityDisables {
    Scope owner_;
    struct Rule {bool self=false;Time applied=0,until=0;ResetSlots reset;};
    Time clock_=0;std::uint64_t revision_=1;std::vector<Rule> rules_;
    bool owns(Scope scope)const{return scope==owner_&&scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
public:
    explicit HumilityDisables(Scope owner):owner_(owner){}
    // Immutable proposal,not a commit. Both participants must be proposed
    // successfully before World replaces either authoritative owner state.
    std::optional<HumilityDisables> propose(Scope owner,const HumilityPlan& plan,bool self,Time now)const{
        const auto canonical=signet_humility(plan.rank);
        if(revision_==UINT64_MAX||!owns(owner)||!canonical||*canonical!=plan||now<clock_||now>UINT64_MAX-std::max(plan.target_disable,plan.self_disable))return {};
        auto result=*this;result.clock_=now;++result.revision_;
        std::erase_if(result.rules_,[&](const auto& rule){return rule.until<=now;});
        const auto until=now+(self?plan.self_disable:plan.target_disable);
        // Merge ordinary overlapping rules, but retain reset layers. A new
        // shorter disable must not restore a previously reset longer one.
        const auto same=std::find_if(result.rules_.begin(),result.rules_.end(),[&](const auto& r){return r.self==self&&r.reset.empty();});
        if(same!=result.rules_.end())same->until=std::max(same->until,until);
        else{if(result.rules_.size()>=128)return {};result.rules_.push_back({self,now,until,{}});}
        return result;
    }
    std::uint64_t revision()const{return revision_;}
    std::optional<HumilityDisables> propose_reset_slots(Scope owner,const std::vector<Slot>& slots,Time now)const{
        const auto keys=validated_reset_slots(slots);
        if(!owns(owner)||!keys||now<clock_||revision_==UINT64_MAX)return {};
        auto result=*this;std::erase_if(result.rules_,[&](const auto& rule){return rule.until<=now;});
        for(auto& rule:result.rules_)rule.reset.insert(keys->begin(),keys->end());
        result.clock_=now;++result.revision_;return result;
    }
    std::optional<Time> disabled_until(Scope owner,Slot slot,Time now)const{
        const auto* row=find_skill(slot.skill);if(!owns(owner)||!slot.skill||!row||slot.copy>65535||now<clock_)return {};
        Time until=0;for(const auto& rule:rules_){
            if(now<rule.applied||now>=rule.until||rule.reset.contains({slot.skill,slot.copy}))continue;
            if(rule.self?row->profession!=5:row->elite())until=std::max(until,rule.until);
        }
        return until;
    }
    std::optional<Time> disabled_until(Scope owner,unsigned skill,Time now)const{return disabled_until(owner,Slot{skill,0},now);}
    void retire(){owner_={};rules_.clear();}

};
struct HumilityAuthorityPair {HumilityDisables caster,target;};
inline std::optional<HumilityAuthorityPair> humility_authority_pair(const HumilityDisables& caster_state,
        const HumilityDisables& target_state,Scope caster,Scope target,const HumilityPlan& plan,Time now){
    if(caster.instance!=target.instance||caster.actor==target.actor)return {};
    auto a=caster_state.propose(caster,plan,true,now),b=target_state.propose(target,plan,false,now);
    if(!a||!b)return {};
    return HumilityAuthorityPair{*a,*b};
}
}
