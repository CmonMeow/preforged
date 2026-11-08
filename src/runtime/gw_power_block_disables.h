#pragma once
#include "gw_power_block.h"
#include "gw_disable_reset.h"

namespace fw::gw::skills::effects {
// World-owned disable authority. Runtime may query any newly equipped skill,
// rather than relying solely on a cooldown packet for an old bar. Caller owns
// synchronization and verifies the active cast before admitting a rule.
class PowerBlockDisables {
    struct Rule {Scope caster;PowerBlockPlan plan;unsigned active;Time applied,until;ResetSlots reset;};
    Scope owner_;
    Time clock_=0;
    std::uint64_t revision_=1;
    std::vector<Rule> rules_;
    bool owns(Scope target)const{return target==owner_&&target.instance&&target.actor&&target.actor<=65535&&target.generation;}
public:
    explicit PowerBlockDisables(Scope owner):owner_(owner){}
    bool apply(Scope target,Scope caster,const PowerBlockPlan& plan,unsigned active,Time now){
        Slots probe{};probe[0]={active,0};
        if(revision_==UINT64_MAX||!owns(target)||!caster.instance||caster.instance!=target.instance||!caster.actor||caster.actor>65535||
           !caster.generation||caster.actor==target.actor||now<clock_||now>UINT64_MAX-plan.disable||
           !power_block_disabled_slots(plan,active,probe))return false;
        auto proposed=rules_;
        std::erase_if(proposed,[&](const auto& rule){return rule.until<=now;});
        if(proposed.size()>=128)return false; // Defensive storage bound,not a retail status limit.
        proposed.push_back({caster,plan,active,now,now+plan.disable});
        rules_=std::move(proposed);clock_=now;++revision_;return true;
    }
    std::uint64_t revision()const{return revision_;}
    std::optional<PowerBlockDisables> propose_reset_slots(Scope target,const std::vector<Slot>& slots,Time now)const{
        const auto keys=validated_reset_slots(slots);
        if(!owns(target)||!keys||now<clock_||revision_==UINT64_MAX)return {};
        auto result=*this;std::erase_if(result.rules_,[&](const auto& rule){return rule.until<=now;});
        for(auto& rule:result.rules_)rule.reset.insert(keys->begin(),keys->end());
        result.clock_=now;++result.revision_;return result;
    }
    std::optional<Time> disabled_until(Scope target,Slot candidate,Time now)const{
        if(!owns(target)||!candidate.skill||!find_skill(candidate.skill)||candidate.copy>65535||now<clock_)return {};
        Slots probe{};probe[0]=candidate;Time until=0;
        for(const auto& rule:rules_){
            if(now<rule.applied||now>=rule.until||rule.reset.contains({candidate.skill,candidate.copy}))continue;
            const auto slots=power_block_disabled_slots(rule.plan,rule.active,probe);
            if(slots&&!slots->empty())until=std::max(until,rule.until);
        }
        return until;
    }
    std::optional<Time> disabled_until(Scope target,unsigned candidate,Time now)const{return disabled_until(target,Slot{candidate,0},now);}
    // Actor retirement clears authority. Caster death is deliberately not an
    // operation here:disabling is a timed effect on this target lifetime.
    void retire(){owner_={};rules_.clear();}
};
}
