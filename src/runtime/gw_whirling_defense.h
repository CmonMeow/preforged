#pragma once
#include "gw_whirling_defense_policy.h"
#include "gw_attack_outcomes.h"

namespace fw::gw::skills::effects {
struct WhirlingRetaliation {
 Scope source;unsigned damage=0,skill=450;Time at=0;float radius=166.f;
};
// The local wiki documents retaliation even when another source blocks the
// projectile. Do not require attribution to450. This is armor-ignoring PHYSICAL
// damage to adjacent foes, not damage to the shooter and not an attack hit.
// Queue exactly once per resolved projectile contact; callers own event dedup.
inline std::optional<WhirlingRetaliation> whirling_retaliation(
 const Registry& registry,Scope defender,combat::weapon::AttackOutcome outcome,
 bool projectile,Time now){
 if(!projectile||outcome!=combat::weapon::AttackOutcome::block||!registry.matches(defender))return {};
 const auto effect=registry.newest_skill(defender,450,now);
 if(!effect||effect->cause!=defender.actor||effect->kind!=Category::stance)return {};
 const auto plan=whirling_defense(effect->rank);if(!plan)return {};
 return WhirlingRetaliation{defender,plan->retaliation,450,now,plan->radius};
}
// Consume authoritative contact-time positions/team/liveness, never client
// supplied distances. Scope generations must also be revalidated at dispatch.
inline bool whirling_retaliation_target(const WhirlingRetaliation& reaction,
 Scope target,bool hostile,bool alive,double distance){
 return reaction.source.instance&&reaction.source.actor&&reaction.source.generation&&
        target.instance==reaction.source.instance&&target.actor&&target.generation&&
        target.actor!=reaction.source.actor&&hostile&&alive&&std::isfinite(distance)&&
        distance>=0&&distance<=reaction.radius;
}
// AttackOutcomes::finish consumes the event before this adapter is called.
// Unknown delivery is not evidence of a projectile, even if !melee was passed
// to an older contact helper.
inline std::optional<WhirlingRetaliation> whirling_resolved_retaliation(
 const Registry& registry,Scope defender,const combat::weapon::ResolvedAttack& event){
 if(!event.event||event.target!=defender||event.at<event.released||
    event.attacker.instance!=defender.instance||!event.attacker.actor||
    !event.attacker.generation||event.attacker.actor==defender.actor)return {};
 return whirling_retaliation(registry,defender,event.outcome,
     event.delivery==combat::weapon::AttackDelivery::ranged,event.at);
}
}
