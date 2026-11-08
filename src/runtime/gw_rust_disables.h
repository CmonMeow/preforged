#pragma once
#include "gw_rust.h"
#include "gw_disable_reset.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
// Target-owned, single active-slot disable. Interrupt success is deliberately
// absent from admission: protection may stop interruption, not this disable.
class RustDisables {
    struct Rule {Slot slot;Time applied,until;};
    Scope owner_;Time clock_=0;std::uint64_t revision_=1;std::vector<Rule> rules_;
    bool owns(Scope scope)const{return scope==owner_&&scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
public:
    explicit RustDisables(Scope owner):owner_(owner){}
    std::uint64_t revision()const{return revision_;}
    std::optional<RustDisables> propose(Scope owner,const RustPlan& plan,RustActivity active,bool overcast,Time now)const{
        const auto canonical=rust(plan.rank);const auto* row=find_skill(active.slot.skill);
        if(!owns(owner)||!canonical||*canonical!=plan||!overcast||!row||row->type!=7||active.slot.copy>65535||
           active.ends<=active.began||now<active.began||now>=active.ends||now<clock_||now>UINT64_MAX-plan.disable||revision_==UINT64_MAX)return {};
        auto result=*this;std::erase_if(result.rules_,[&](const auto& r){return r.until<=now;});
        auto same=std::find_if(result.rules_.begin(),result.rules_.end(),[&](const auto& r){return r.slot==active.slot;});
        if(same!=result.rules_.end())same->until=std::max(same->until,now+plan.disable);
        else{if(result.rules_.size()>=128)return {};result.rules_.push_back({active.slot,now,now+plan.disable});}
        result.clock_=now;++result.revision_;return result;
    }
    std::optional<Time> disabled_until(Scope owner,Slot slot,Time now)const{
        if(!owns(owner)||!slot.skill||!find_skill(slot.skill)||slot.copy>65535||now<clock_)return {};
        Time until=0;for(const auto& r:rules_)if(r.slot==slot&&r.applied<=now&&now<r.until)until=std::max(until,r.until);
        return until;
    }
    std::optional<RustDisables> propose_reset_slots(Scope owner,const std::vector<Slot>& slots,Time now)const{
        const auto keys=validated_reset_slots(slots);if(!owns(owner)||!keys||now<clock_||revision_==UINT64_MAX)return {};
        auto result=*this;std::erase_if(result.rules_,[&](const auto& r){return r.until<=now||keys->contains({r.slot.skill,r.slot.copy});});
        result.clock_=now;++result.revision_;return result;
    }
    void retire(){owner_={};rules_.clear();}
};
}
