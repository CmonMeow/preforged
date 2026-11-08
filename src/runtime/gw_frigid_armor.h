#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <optional>

namespace fw::gw::skills::effects {
struct FrigidArmorPlan {
    unsigned rank=0,physical_armor=10;
    Time duration=10000;
    bool burning_immunity=true;
    bool operator==(const FrigidArmorPlan&)const=default;
};

inline std::optional<FrigidArmorPlan> frigid_armor(unsigned rank){
    const auto* row=find_skill(1261);
    if(rank>20||!row||row->campaign!=3||row->profession!=6||row->attribute!=11||
       row->type!=6||row->target||row->energy_cost()!=5||row->health_sacrifice||
       row->adrenaline||row->activation!=1.f||row->aftercast!=.75f||
       row->recharge!=20||row->scale0!=10||row->scale15!=40||
       row->duration0!=10||row->duration15!=25||row->caster_body_resource!=369||
       row->target_body_resource!=2077)return {};
    return FrigidArmorPlan{rank,interpolate(10,40,rank),
        1000ull*interpolate(10,25,rank),true};
}

struct FrigidArmorApplication {
    Registry registry;
    Entry applied;
    std::vector<Entry> replaced;
};
inline std::optional<FrigidArmorApplication> prepare_frigid_armor(
        const Registry& current,Scope owner,unsigned effect,
        const FrigidArmorPlan& plan,Time now){
    if(frigid_armor(plan.rank)!=plan||!owner.instance||!owner.actor||
       !owner.generation||!current.matches(owner)||!effect||
       now>UINT64_MAX-plan.duration)return {};
    Registry proposed=current;
    const Entry applied{effect,owner.actor,1261,plan.rank,
        Category::enchantment,now,now+plan.duration};
    const auto result=proposed.apply(owner,applied,Replace::same_skill);
    if(!result.accepted)return {};
    return FrigidArmorApplication{std::move(proposed),applied,result.removed};
}

inline std::optional<FrigidArmorPlan> active_frigid_armor(
        const Registry& registry,Scope owner,Time now){
    const auto entry=registry.newest_skill(owner,1261,now);
    if(!entry||entry->kind!=Category::enchantment||!entry->instance||
       entry->cause!=owner.actor||!entry->expires)return {};
    const auto plan=frigid_armor(entry->rank);
    if(!plan||entry->applied>UINT64_MAX-plan->duration||
       *entry->expires<entry->applied+plan->duration)return {};
    // The host validates the exact EnchantmentDuration quote on insertion.
    // Keep active lookup compatible with the reviewed 10..20% weapon suffix;
    // the registry itself cannot retain that transient equipment receipt.
    const auto seconds=plan->duration/1000;
    const auto max_bonus=std::max(Time(1),(seconds*20+50)/100);
    const auto max_duration=(seconds+max_bonus)*1000;
    if(entry->applied>UINT64_MAX-max_duration||
       *entry->expires>entry->applied+max_duration)return {};
    return plan;
}
}
