#pragma once
#include "gw_selfless_spirit.h"
#include "gw_enchantment_duration.h"

namespace fw::gw::skills::effects {
inline bool selfless_spirit_alias(unsigned skill){return skill==1952||skill==2095;}
inline bool valid_selfless_owner(Scope owner){
    return owner.instance&&owner.actor&&owner.actor<=65535&&owner.generation;
}

inline std::optional<Entry> prepare_selfless_spirit_entry(Scope owner,unsigned effect,
        const SelflessSpiritPlan& plan,Time now,const EnchantmentDuration* adjusted=nullptr){
    if(!valid_selfless_owner(owner)||!effect||selfless_spirit(plan.skill,plan.rank)!=plan)return {};
    const auto duration=adjusted?adjusted->for_enchantment(plan.skill,plan.duration_ms,owner,now)
        :std::optional<Time>{plan.duration_ms};
    if(!duration||!*duration||now>UINT64_MAX-*duration)return {};
    return Entry{effect,owner.actor,plan.skill,plan.rank,Category::enchantment,now,now+*duration};
}

struct SelflessSpiritRegistryStage {
    Registry registry;
    Entry added;
    std::vector<Entry> removed;
};
// The original remains untouched even if validation, allocation or apply fails.
// The caller commits this proposal together with matching World health_effects,
// effect allocation, duration receipt and publication transaction.
// Input is a current World registry: no future-dated applications are permitted.
// This helper does not discover future entries through Registry's active API.
inline std::optional<SelflessSpiritRegistryStage> stage_selfless_spirit_registry(
        const Registry& current,Scope owner,unsigned effect,const SelflessSpiritPlan& plan,
        Time now,const EnchantmentDuration* adjusted=nullptr){
    if(!current.matches(owner))return {};
    const auto entry=prepare_selfless_spirit_entry(owner,effect,plan,now,adjusted);
    if(!entry)return {};
    // A new allocator receipt may not overwrite another effect's identity.
    if(current.lookup(owner,effect))return {};
    SelflessSpiritRegistryStage stage{current,*entry,{}};
    // Raw allocation enumeration is unavailable, so retire expired entries on
    // the COPY first.
    // Return every retired entry for the caller's lifecycle publication.
    stage.removed=stage.registry.expire(owner,now);
    for(const auto& old:stage.registry.active(owner,now))if(selfless_spirit_alias(old.skill)){
        stage.registry.remove(owner,old.instance);stage.removed.push_back(old);
    }
    const auto applied=stage.registry.apply(owner,*entry,Replace::independent);
    if(!applied.accepted)return {};
    stage.removed.insert(stage.removed.end(),applied.removed.begin(),applied.removed.end());
    return stage;
}

// No target semantics or Energy pricing are inferred here. A successful empty
// optional means absence; outer nullopt means invalid ownership/registry data.
inline std::optional<std::optional<Entry>> live_selfless_spirit_entry(
        const Registry& registry,Scope owner,Time now){
    if(!valid_selfless_owner(owner)||!registry.matches(owner))return {};
    const auto entry=registry.newest_skill(owner,1952,now,2095);
    if(!entry)return std::optional<Entry>{};
    if(!selfless_spirit(entry->skill,entry->rank)||entry->cause!=owner.actor||
       entry->kind!=Category::enchantment||!entry->expires||*entry->expires<=entry->applied)return {};
    return entry;
}
}
