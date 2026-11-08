#pragma once
#include "gw_npc_combat_catalog.h"
#include "gw_attack_interrupt_disable.h"
namespace fw::gw::combat::npcs {
struct AttackInterruptProposal {
 LiveActor actor;weapon::AttackInterruptDisables disables;
 weapon::InterruptedActionReceipt receipt;
};
// Prepare both normal recharge/cancellation and independent disable without
// touching the original actor. A fabricated animation/manual windup is not a
// reviewed spellbook action and cannot manufacture a recharge receipt.
inline std::optional<AttackInterruptProposal> prepare_attack_interrupt(const LiveActor& current,
 const Lifetime& life,const control::Token& token,const weapon::AttackInterruptDisables& disables,
 const weapon::InterruptDisablePlan& plan,std::uint64_t now){
 const auto active=current.active_action(life,now);
 if(!active||*active!=token||token.kind!=control::Action::spell||!token.skill||
    !current.owns_spell_action(life,token)||!current.spell_recharge_until(life,*token.skill,now))return {};
 auto actor=current;if(!actor.cancel_action(life,now))return {};
 const auto recharge=actor.spell_recharge_until(life,*token.skill,now);if(!recharge||*recharge<now)return {};
 const weapon::InterruptedActionReceipt receipt{current.hex_scope(),*token.skill,token.sequence,token.begins,token.completes,now,*recharge};
 const auto proposed=disables.propose(current.hex_scope(),plan,receipt,now);if(!proposed)return {};
 return AttackInterruptProposal{std::move(actor),*proposed,receipt};
}
}
