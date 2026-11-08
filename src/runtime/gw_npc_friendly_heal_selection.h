#pragma once
#include "gw_npc_friendly_heal.h"
#include <span>
namespace fw::gw::combat::ai::friendly {
enum class HealPriority {declared_order,lowest_fraction};
struct HealPolicy {
    unsigned threshold_per_mille=0;
    HealPriority priority=HealPriority::declared_order;
    bool hold_position=true;
    float maximum_follow=0;
    std::string evidence;
};
enum class HealDecisionKind {cast,approach};
struct HealDecision {HealDecisionKind kind;skills::effects::Scope target;};
inline bool valid_policy(const HealPolicy& policy){
    return policy.threshold_per_mille>0&&policy.threshold_per_mille<1000&&
        (policy.priority==HealPriority::declared_order||policy.priority==HealPriority::lowest_fraction)&&
        std::isfinite(policy.maximum_follow)&&policy.maximum_follow>=0&&
        (policy.hold_position||policy.maximum_follow>0)&&!policy.evidence.empty();
}
// All ordering/threshold/movement decisions are caller-authored policy. Capture
// capability does not establish a universal retail NPC healing priority.
inline std::optional<HealDecision> select(const HealPolicy& policy,const HealProfile& profile,
        const Healer& source,std::span<const Recipient> candidates,std::uint64_t now,
        bool action_active,bool spell_available){
    if(!valid_policy(policy)||action_active||!spell_available)return {};
    const auto heal=skills::effects::direct_heal(profile.skill);if(!heal)return {};
    const Recipient* best=nullptr;
    for(const auto& recipient:candidates){
        if(!valid(recipient.health)||!std::isfinite(recipient.distance)||recipient.distance<0||
           recipient.health.current*1000>=double(recipient.health.maximum)*policy.threshold_per_mille)continue;
        const bool in_range=recipient.distance<=heal->range;
        if(!in_range&&(policy.hold_position||recipient.distance>policy.maximum_follow))continue;
        auto admissible_recipient=recipient;
        if(!in_range)admissible_recipient.distance=0; // inspect effect eligibility without inventing a cast in range
        if(!admissible(profile,source,admissible_recipient,now))continue;
        if(!best){best=&recipient;continue;}
        if(policy.priority==HealPriority::lowest_fraction){
            const auto a=recipient.health.current*best->health.maximum,b=best->health.current*recipient.health.maximum;
            if(a<b||(a==b&&recipient.scope.actor<best->scope.actor))best=&recipient;
        }
    }
    if(!best)return {};
    return HealDecision{best->distance<=heal->range?HealDecisionKind::cast:HealDecisionKind::approach,best->scope};
}
}
