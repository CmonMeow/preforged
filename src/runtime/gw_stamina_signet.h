#pragma once
#include "gw_attack_outcomes.h"
#include "gw_skill_effect_plan.h"

namespace fw::gw::skills::effects {
struct StaminaSignetPlan {
    unsigned rank=0,health_bonus=50;
    Time activation=250,aftercast=750,recharge=20000;
    bool operator==(const StaminaSignetPlan&)const=default;
};

inline std::optional<StaminaSignetPlan> stamina_signet(unsigned rank){
    const auto* row=find_skill(1411);
    if(rank>20||!row||row->type!=7||row->profession!=1||row->attribute!=17||
       row->energy_cost()||row->adrenaline||row->health_sacrifice||
       row->activation!=.25f||row->aftercast!=.75f||row->recharge!=20||
       row->scale0!=50||row->scale15!=300||row->duration0!=196608||
       row->duration15!=196608||row->caster_body_resource!=1544||
       row->target_body_resource!=2077)return {};
    return StaminaSignetPlan{rank,interpolate(50,300,rank)};
}

inline std::optional<Entry> active_stamina_signet(const Registry& registry,Scope owner,Time now){
    const auto entry=registry.newest_skill(owner,1411,now);
    if(!entry||entry->kind!=Category::other||entry->cause!=owner.actor||
       entry->expires||!stamina_signet(entry->rank))return {};
    return entry;
}

// The hit must be server-qualified, already associated with the same actor
// lifetime, and released after this exact effect was applied. A miss, block,
// dodge or cancelled attack never reaches this consumer.
inline std::optional<Entry> end_stamina_on_hit(Registry& registry,Scope owner,
                                               const combat::weapon::QualifiedHit& hit){
    const auto entry=active_stamina_signet(registry,owner,hit.at);
    if(!entry||hit.attacker!=owner||hit.target.instance!=owner.instance||
       !hit.target.actor||hit.target.actor>65535||hit.target.actor==owner.actor||!hit.target.generation||
       !hit.event||hit.released<entry->applied||hit.at<hit.released||
       hit.delivery==combat::weapon::AttackDelivery::unknown)return {};
    return registry.remove(owner,entry->instance);
}
}
