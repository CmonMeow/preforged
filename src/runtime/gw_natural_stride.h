#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"

namespace fw::gw::skills::effects {
struct NaturalStridePlan {
    unsigned rank=0;
    Time duration=1000;
    float movement=1.33f;
    unsigned block_percent=50;
    auto operator<=>(const NaturalStridePlan&)const=default;
};
inline std::optional<NaturalStridePlan> natural_stride(unsigned rank){
    const auto* row=find_skill(1727);
    if(rank>20||!row||row->campaign!=3||row->profession!=2||
       row->attribute!=24||row->type!=3||row->energy_cost()!=5||
       row->activation!=0.f||row->aftercast!=0.f||row->recharge!=12||
       row->scale0!=33||row->scale15!=33||row->bonus0!=50||row->bonus15!=50||
       row->duration0!=1||row->duration15!=8||row->caster_body_resource!=724||
       row->target_body_resource!=2077)return {};
    return NaturalStridePlan{rank,1000ull*interpolate(1,8,rank)};
}
inline std::optional<NaturalStridePlan> natural_stride_entry(const Entry& entry,Time now){
    if(entry.skill!=1727||entry.kind!=Category::stance||!entry.instance||!entry.cause||
       entry.applied>now||!entry.expires||now>=*entry.expires)return {};
    const auto plan=natural_stride(entry.rank);
    if(!plan||entry.applied>UINT64_MAX-plan->duration||
       *entry.expires!=entry.applied+plan->duration)return {};
    return plan;
}
inline std::optional<NaturalStridePlan> active_natural_stride(
        const Registry& registry,Scope owner,Time now){
    const auto entry=registry.newest(owner,Category::stance,now);
    if(!entry||entry->skill!=1727||!entry->instance||!entry->cause)return {};
    const auto plan=natural_stride(entry->rank);
    if(!plan||entry->applied>UINT64_MAX-plan->duration||
       entry->expires!=std::optional<Time>{entry->applied+plan->duration})return {};
    return plan;
}
// Arm Registry::observe_applications(owner,1727) before applying the stance.
// Only committed new hex/enchantment applications end it. Existing effects
// are intentionally irrelevant, including ones active at stance activation.
// The observer snapshot supplies exact instance/cause, preserving same-tick
// order and preventing an old queued application from ending a replacement.
inline std::optional<Entry> natural_stride_application(
        Registry& registry,Scope owner,const ApplicationEvent& event){
    if(!registry.matches(owner)||event.target!=owner||!event.sequence||
       (event.applied.kind!=Category::hex&&event.applied.kind!=Category::enchantment))return {};
    for(const auto& observer:event.observers){
        if(observer.skill!=1727||observer.kind!=Category::stance||
           !observer.instance||!observer.cause)continue;
        const auto plan=natural_stride(observer.rank);
        if(!plan||observer.applied>UINT64_MAX-plan->duration||
           observer.expires!=std::optional<Time>{observer.applied+plan->duration}||
           event.applied.applied<observer.applied||
           event.applied.applied>=*observer.expires)continue;
        const auto current=registry.lookup(owner,observer.instance);
        if(current&&current->skill==1727&&current->cause==observer.cause&&
           current->applied==observer.applied&&current->expires==observer.expires)
            return registry.remove(owner,observer.instance);
    }
    return {};
}
}
