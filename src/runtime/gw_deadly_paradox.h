#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_cooldown_bar_authority.h"
#include "gw_disable_reset.h"
#include <algorithm>
#include <optional>
#include <vector>

namespace fw::gw::skills::effects {
struct DeadlyParadoxPlan {
    unsigned rank=0;
    Time duration=5000;
    Time attack_disable=10000;
    auto operator<=>(const DeadlyParadoxPlan&) const = default;
};
inline std::optional<DeadlyParadoxPlan> deadly_paradox(unsigned rank){
    const auto* row=find_skill(572);
    if(rank>20||!row||row->type!=3||row->profession!=7||row->attribute!=30||
       row->energy_cost()!=15||row->activation||row->aftercast||row->recharge!=10||
       row->scale0!=33||row->scale15!=33||row->bonus0!=10||row->bonus15!=10||
       row->duration0!=5||row->duration15!=15||row->caster_body_resource!=1360)return {};
    return DeadlyParadoxPlan{rank,1000ull*interpolate(5,15,rank),10000};
}
inline bool active_deadly_paradox(const Entry& source,Time now){
    return source.skill==572&&source.kind==Category::stance&&source.instance&&source.cause&&
           source.applied<=now&&source.expires&&now<*source.expires&&
           deadly_paradox(source.rank)&&*source.expires-source.applied==deadly_paradox(source.rank)->duration;
}
// The fixed local cast leaves its own 10-second recharge unchanged, while
// Return [769] recharges in 7 seconds. Dagger attacks still gain recharge.
inline bool deadly_paradox_recharges(const SkillRecord& skill,const Entry& source,Time now){
    return active_deadly_paradox(source,now)&&skill.profession==7&&skill.id!=572;
}
// The local Return cast finishes after 188ms from a 250ms base. The older
// wiki documents the dagger-attack activation exception.
inline bool deadly_paradox_activates(const SkillRecord& skill,const Entry& source,Time now){
    return deadly_paradox_recharges(skill,source,now)&&skill.type!=14;
}

inline bool deadly_paradox_attack_skill(unsigned skill){
    const auto* row=find_skill(skill);
    return row&&row->id&&(row->type==14||row->type==20);
}

// Attack disables outlive the stance. Bind them to exact skillbar copies so
// replacing a slot cannot silently transfer the old timer to a new skill.
class DeadlyParadoxDisables {
    struct Rule {Slot slot;Time until=0;};
    Scope owner_;
    Time clock_=0;
    std::uint64_t revision_=1,last_event_=0;
    std::vector<Rule> rules_;
public:
    explicit DeadlyParadoxDisables(Scope owner):owner_(owner){}
    std::uint64_t revision()const{return revision_;}
    std::optional<DeadlyParadoxDisables> propose(Scope owner,const DeadlyParadoxPlan& plan,
            const CooldownBarSnapshot& bar,std::uint64_t event,Time now)const{
        if(owner!=owner_||!cooldown_owner(owner)||deadly_paradox(plan.rank)!=plan||
           !event||event<=last_event_||now<clock_||now>UINT64_MAX-plan.attack_disable||
           revision_==UINT64_MAX||bar.published.owner!=owner||!bar.published.order||
           !bar.revision||bar.published.at>now||
           !std::any_of(bar.bar.begin(),bar.bar.end(),[](Slot slot){return slot.skill==572;}))return {};
        auto next=*this;
        std::erase_if(next.rules_,[&](const Rule& rule){return rule.until<=now;});
        ResetSlots seen;
        for(const auto slot:bar.bar){
            if(!slot.skill){if(slot.copy)return {};continue;}
            if(!cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
            if(!deadly_paradox_attack_skill(slot.skill))continue;
            const auto found=std::find_if(next.rules_.begin(),next.rules_.end(),
                [&](const Rule& rule){return rule.slot==slot;});
            if(found==next.rules_.end()){if(next.rules_.size()>=128)return {};
                next.rules_.push_back({slot,now+plan.attack_disable});}
            else found->until=std::max(found->until,now+plan.attack_disable);
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
    std::optional<DeadlyParadoxDisables> propose_reset_slots(Scope owner,
            const std::vector<Slot>& slots,Time now)const{
        const auto valid=validated_reset_slots(slots);
        if(owner!=owner_||!cooldown_owner(owner)||!valid||now<clock_||revision_==UINT64_MAX)return {};
        auto next=*this;
        std::erase_if(next.rules_,[&](const Rule& rule){
            return rule.until<=now||valid->contains({rule.slot.skill,rule.slot.copy});
        });
        next.clock_=now;++next.revision_;return next;
    }
};
}
