#pragma once
#include "gw_healing_ring.h"
#include "gw_npc_combat_catalog.h"
namespace fw::gw::skills::effects {
// Caller owns range/plane selection and must settle actual NPC timeline events
// before committing this value proposal. This function never publishes or
// guesses spirit immunity from a model, allegiance or summoned flag.
inline std::optional<combat::npcs::LiveActor> prepare_healing_ring_npc(const HealingRingPlan& plan,
 Scope caster,const RingRecipient& recipient,RingCreature eligibility,
 const combat::npcs::LiveActor& original,combat::npcs::Lifetime expected,Time now){
 const auto canonical=healing_ring(plan.rank,plan.divine_rank);
 if(!canonical||*canonical!=plan||eligibility!=RingCreature::creature||
    !caster.instance||!caster.actor||caster.actor>65535||!caster.generation||
    recipient.scope.instance!=caster.instance||recipient.scope.actor==caster.actor||
    recipient.scope!=original.hex_scope()||!original.matches(expected)||original.healing_allowed(expected)!=std::optional<bool>(true)||recipient.healing!=plan.healing)return {};
 auto proposal=original;
 if(!proposal.heal(expected,recipient.healing,now))return {};
 return proposal;
}
}
