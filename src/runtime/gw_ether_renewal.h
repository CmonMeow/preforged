#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_aura_restoration.h"
#include <optional>

namespace fw::gw::skills::effects {
struct EtherRenewalPlan {
    unsigned skill=181,rank=0,energy_per_enchantment=1,healing_per_enchantment=5;
    Time duration=5000;
    bool operator==(const EtherRenewalPlan&)const=default;
};
inline std::optional<EtherRenewalPlan> ether_renewal(unsigned skill,unsigned rank){
    const auto* row=find_skill(skill);
    const bool pvp=skill==2860;
    if((skill!=181&&!pvp)||rank>20||!row||row->campaign!=1||
       row->profession!=6||row->attribute!=12||row->type!=6||row->target||
       row->energy_cost()!=10||row->activation!=1.f||row->aftercast!=.75f||
       row->recharge!=30||row->adrenaline||row->health_sacrifice||
       row->scale0!=1||row->scale15!=(pvp?3u:4u)||row->bonus0!=5||row->bonus15!=20||
       row->duration0!=(pvp?7u:5u)||row->duration15!=(pvp?7u:20u)||
       row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
    return EtherRenewalPlan{skill,rank,interpolate(1,pvp?3:4,rank),
        interpolate(5,20,rank),pvp?7000:1000ull*interpolate(5,20,rank)};
}
struct EtherRenewalApplication {
    Registry registry;Entry applied;std::vector<Entry> replaced;
};
inline std::optional<EtherRenewalApplication> prepare_ether_renewal(
        const Registry& current,Scope owner,unsigned effect,const EtherRenewalPlan& plan,Time now){
    if(ether_renewal(plan.skill,plan.rank)!=plan||!owner.instance||!owner.actor||
       !owner.generation||!current.matches(owner)||!effect||now>UINT64_MAX-plan.duration)return {};
    auto proposed=current;
    // A mode split is one effect family. Replacing a mode variant must not
    // leave the other version installed in a copied authoritative registry.
    std::vector<Entry> removed;
    for(const auto& entry:proposed.active(owner,now))if(entry.skill==181||entry.skill==2860){
        removed.push_back(entry);proposed.remove(owner,entry.instance);
    }
    const Entry applied{effect,owner.actor,plan.skill,plan.rank,Category::enchantment,now,now+plan.duration};
    auto result=proposed.apply(owner,applied,Replace::same_skill);
    if(!result.accepted)return {};
    removed.insert(removed.end(),result.removed.begin(),result.removed.end());
    return EtherRenewalApplication{std::move(proposed),applied,std::move(removed)};
}
struct EtherRenewalReward {
    Entry effect;unsigned enchantments=0,energy=0,healing=0;
};
// Pure preflight on a caller-owned completion snapshot. This does not establish
// successful casting, event uniqueness, resource capacity or reaction order;
// the World completion authority must own those checks before committing.
inline std::optional<EtherRenewalReward> ether_renewal_reward(
        const Registry& registry,Scope owner,unsigned cast_skill,Time now){
    const auto* cast=find_skill(cast_skill);
    if(!cast||!is_spell(*cast)||!registry.matches(owner))return {};
    const auto entry=registry.newest_skill(owner,181,now,2860);
    if(!entry||entry->cause!=owner.actor||entry->kind!=Category::enchantment||
       !entry->instance||!entry->expires)return {};
    const auto plan=ether_renewal(entry->skill,entry->rank);
    if(!plan||entry->applied>UINT64_MAX-plan->duration||*entry->expires<entry->applied+plan->duration)return {};
    const auto seconds=plan->duration/1000;
    const auto maximum=(seconds+std::max(Time(1),(seconds*20+50)/100))*1000;
    if(entry->applied>UINT64_MAX-maximum||*entry->expires>entry->applied+maximum)return {};
    const auto count=registry.count(owner,Category::enchantment,now);
    if(!count||count>128)return {};
    return EtherRenewalReward{*entry,static_cast<unsigned>(count),
        static_cast<unsigned>(count)*plan->energy_per_enchantment,
        static_cast<unsigned>(count)*plan->healing_per_enchantment};
}
}
