#pragma once
#include "gw_gladiator_defense_policy.h"
#include "gw_block_attribution.h"

namespace fw::gw::skills::effects {
struct GladiatorRetaliation {Scope source,target;unsigned damage=0,skill=372;Time at=0;};
// Inputs must come from the resolved contact and current generation-owned
// registry. A block by any other source is not a Gladiator's Defense trigger.
inline std::optional<GladiatorRetaliation> gladiator_retaliation(
 const Registry& registry,Scope defender,Scope attacker,const AttributedBlock& contact,
 bool melee,Time now){
 if(!melee||!registry.matches(defender)||!attacker.instance||attacker.instance!=defender.instance||
    !attacker.actor||!attacker.generation||attacker.actor==defender.actor||
    contact.outcome!=combat::weapon::AttackOutcome::block||!contact.source||
    contact.source->skill!=372||!contact.source->instance||contact.source->probability!=.75)return {};
 const auto effect=registry.newest_skill(defender,372,now);
 if(!effect||effect->instance!=contact.source->instance||effect->cause!=defender.actor||
    effect->kind!=Category::stance)return {};
 const auto plan=gladiator_defense(effect->rank);if(!plan)return {};
 return GladiatorRetaliation{defender,attacker,plan->retaliation,372,now};
}
}
