#pragma once
#include "gw_heal_effects.h"
#include "gw_cooldown_bar_authority.h"
#include "gw_disable_reset.h"

namespace fw::gw::skills::effects {
struct HealingBurstPlan {
    unsigned rank=0, heal=10, divine_party_heal=0;
    Time smiting_disable=20000;
    float target_range=1248.f, party_range=1000.f;
    auto operator<=>(const HealingBurstPlan&)const=default;
};
inline std::optional<HealingBurstPlan> healing_burst(unsigned rank,unsigned divine){
    const auto* row=find_skill(1118);
    if(!row||rank>20||divine>20||row->campaign!=2||row->profession!=3||
       row->attribute!=13||row->type!=5||!row->elite()||
       row->energy_cost()!=5||row->activation!=.75f||row->aftercast!=.75f||
       row->recharge!=4||row->scale0!=10||row->scale15!=160||
       row->caster_body_resource!=2077||row->target_body_resource!=1256)return {};
    return HealingBurstPlan{rank,heal_scale(10,160,rank),
        static_cast<unsigned>(std::lround(3.2*divine))};
}
inline bool valid_healing_burst(const HealingBurstPlan& plan){
    for(unsigned divine=0;divine<=20;++divine)
        if(healing_burst(plan.rank,divine)==plan)return true;
    return false;
}
inline std::optional<unsigned> healing_burst_recipient(const HealingBurstPlan& plan,
        bool selected,bool in_caster_party,float distance_from_target){
    if(!valid_healing_burst(plan)||!std::isfinite(distance_from_target)||
       distance_from_target<0)return {};
    if(selected)return plan.heal+plan.divine_party_heal;
    return in_caster_party&&distance_from_target<=plan.party_range?
        plan.divine_party_heal:0u;
}

// Successful completion disables every Smiting Prayers slot on the caster's
// authoritative bar. Exact skill copies keep independent disable state.
class HealingBurstDisables {
    struct Rule {Slot slot;Time until=0;bool operator==(const Rule&)const=default;};
    Scope owner_;Time clock_=0;std::uint64_t revision_=1,last_event_=0;
    std::vector<Rule> rules_;
public:
    explicit HealingBurstDisables(Scope owner):owner_(owner){}
    std::uint64_t revision()const{return revision_;}
    std::optional<HealingBurstDisables> propose(Scope owner,const HealingBurstPlan& plan,
            const CooldownBarSnapshot& bar,std::uint64_t event,Time now)const{
        if(owner!=owner_||!cooldown_owner(owner)||!valid_healing_burst(plan)||
           !event||event<=last_event_||now<clock_||
           now>UINT64_MAX-plan.smiting_disable||revision_==UINT64_MAX||
           bar.published.owner!=owner||!bar.published.order||!bar.revision||
           bar.published.at>now)return {};
        auto next=*this;
        std::erase_if(next.rules_,[&](const auto& rule){return rule.until<=now;});
        ResetSlots seen;
        for(const auto slot:bar.bar){
            if(!slot.skill){if(slot.copy)return {};continue;}
            if(!cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
            const auto* row=find_skill(slot.skill);
            if(!row||row->attribute!=14)continue;
            auto found=std::find_if(next.rules_.begin(),next.rules_.end(),
                [&](const auto& rule){return rule.slot==slot;});
            if(found==next.rules_.end()){
                if(next.rules_.size()>=128)return {};
                next.rules_.push_back({slot,now+plan.smiting_disable});
            }else found->until=std::max(found->until,now+plan.smiting_disable);
        }
        next.clock_=now;next.last_event_=event;++next.revision_;
        return next;
    }
    std::optional<Time> disabled_until(Scope owner,Slot slot,Time now)const{
        if(owner!=owner_||!cooldown_owner(owner)||!cooldown_slot(slot)||now<clock_)return {};
        Time until=0;
        for(const auto& rule:rules_)if(rule.slot==slot&&now<rule.until)
            until=std::max(until,rule.until);
        return until;
    }
    std::optional<HealingBurstDisables> propose_reset_slots(Scope owner,
            const std::vector<Slot>& slots,Time now)const{
        const auto valid=validated_reset_slots(slots);
        if(owner!=owner_||!cooldown_owner(owner)||!valid||now<clock_||
           revision_==UINT64_MAX)return {};
        auto next=*this;
        std::erase_if(next.rules_,[&](const auto& rule){
            return rule.until<=now||valid->contains({rule.slot.skill,rule.slot.copy});
        });
        next.clock_=now;++next.revision_;return next;
    }
    void retire(){owner_={};rules_.clear();}
};
}
