#pragma once
#include "gw_cooldown_bar_authority.h"
#include "gw_disable_reset.h"
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"

namespace fw::gw::skills::effects {
struct BestialFuryPlan {
    unsigned rank=0;
    Time duration=5000, non_attack_disable=5000;
    unsigned attack_speed_percent=25;
    auto operator<=>(const BestialFuryPlan&)const=default;
};

inline std::optional<BestialFuryPlan> bestial_fury(unsigned rank){
    const auto* row=find_skill(1209);
    if(rank>20||!row||row->campaign!=2||row->profession!=2||
       row->attribute!=22||row->type!=3||row->special||
       row->energy_cost()!=10||row->adrenaline||row->activation||row->aftercast||
       row->recharge!=10||row->duration0!=5||row->duration15!=11||
       row->scale0!=25||row->scale15!=25||
       row->caster_body_resource!=766||row->target_body_resource!=2077)return {};
    return BestialFuryPlan{rank,1000ull*interpolate(5,11,rank)};
}

inline bool bestial_fury_non_attack(unsigned skill){
    const auto* row=find_skill(skill);
    return row&&row->id&&row->type&&row->type!=14&&row->type!=20;
}

inline bool bestial_fury_active(std::span<const Entry> entries,Time now){
    for(const auto& entry:entries)
        if(entry.skill==1209&&entry.kind==Category::stance&&entry.instance&&
           entry.cause&&entry.applied<=now&&entry.expires&&now<*entry.expires&&
           bestial_fury(entry.rank))return true;
    return false;
}

// The disable is attached to exact bar copies, including the stance itself.
// A later bar publication cannot silently transfer a disable to a different copy.
class BestialFuryDisables {
    struct Rule {Slot slot;Time until=0;bool operator==(const Rule&)const=default;};
    Scope owner_;Time clock_=0;std::uint64_t revision_=1,last_event_=0;
    std::vector<Rule> rules_;
public:
    explicit BestialFuryDisables(Scope owner):owner_(owner){}
    std::uint64_t revision()const{return revision_;}
    std::optional<BestialFuryDisables> propose(Scope owner,const BestialFuryPlan& plan,
            const CooldownBarSnapshot& bar,std::uint64_t event,Time now)const{
        if(owner!=owner_||!cooldown_owner(owner)||bestial_fury(plan.rank)!=plan||
           !event||event<=last_event_||now<clock_||
           now>UINT64_MAX-plan.non_attack_disable||revision_==UINT64_MAX||
           bar.published.owner!=owner||!bar.published.order||!bar.revision||
           bar.published.at>now)return {};
        auto next=*this;
        std::erase_if(next.rules_,[&](const auto& r){return r.until<=now;});
        ResetSlots seen;
        for(const auto slot:bar.bar){
            if(!slot.skill){if(slot.copy)return {};continue;}
            if(!cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
            if(!bestial_fury_non_attack(slot.skill))continue;
            auto found=std::find_if(next.rules_.begin(),next.rules_.end(),
                [&](const auto& r){return r.slot==slot;});
            if(found==next.rules_.end())next.rules_.push_back({slot,now+plan.non_attack_disable});
            else found->until=std::max(found->until,now+plan.non_attack_disable);
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
    std::optional<BestialFuryDisables> propose_reset_slots(Scope owner,
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
