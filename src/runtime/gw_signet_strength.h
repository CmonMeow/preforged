#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <compare>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace fw::gw::skills::effects {
struct SignetStrengthPlan {
    unsigned rank=0,charges=1,bonus_damage=5;
    unsigned activation_ms=1000,aftercast_ms=750,recharge_ms=45000;
    auto operator<=>(const SignetStrengthPlan&)const=default;
};

inline std::optional<SignetStrengthPlan> signet_strength(unsigned rank){
    const auto* row=find_skill(944);
    if(rank>20||!row||row->campaign!=2||row->profession!=1||
       row->attribute!=17||row->type!=7||row->energy_cost()!=0||
       row->health_sacrifice||row->adrenaline||row->activation!=1.f||
       row->aftercast!=.75f||row->recharge!=45||row->scale0!=1||
       row->scale15!=16||row->bonus0!=5||row->bonus15!=5||
       row->caster_body_resource!=1028||row->target_body_resource!=2077)return {};
    return SignetStrengthPlan{rank,rank+1,5};
}

struct StrengthCharges {
    Scope owner;
    unsigned effect=0,remaining=0;
};
struct StrengthContact {
    Scope foe;
    bool landed=false;
};
struct StrengthResolution {
    StrengthCharges next;
    std::vector<unsigned> bonus_by_contact;
};

// Contacts are one ordered weapon action, not a count of swings. World must
// supply distinct, authorized foes and actual post-block hit results. A miss
// or block does not consume a charge; each foe actually struck does.
inline std::optional<StrengthResolution> signet_strength_contacts(
        const SignetStrengthPlan& plan,StrengthCharges current,
        std::span<const StrengthContact> contacts){
    if(signet_strength(plan.rank)!=plan||!current.owner.instance||
       !current.owner.actor||!current.owner.generation||!current.effect||
       current.remaining>plan.charges||contacts.size()>4096)return {};
    StrengthResolution result{current,{}};
    result.bonus_by_contact.reserve(contacts.size());
    std::set<unsigned> seen;
    for(const auto& contact:contacts){
        if(contact.foe.instance!=current.owner.instance||!contact.foe.actor||
           !contact.foe.generation||contact.foe.actor==current.owner.actor||
           !seen.insert(contact.foe.actor).second)return {};
        if(contact.landed&&result.next.remaining){
            --result.next.remaining;
            result.bonus_by_contact.push_back(plan.bonus_damage);
        }else result.bonus_by_contact.push_back(0);
    }
    return result;
}
}
