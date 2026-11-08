#pragma once
#include "gw_attack_interrupts.h"
#include "gw_attack_outcomes.h"
namespace fw::gw::combat::weapon {
struct SkullCrackPlan {
 unsigned skill=329,adrenaline=220,target_body=576;
 std::uint64_t windup=500,dazed_ms=15000;
 bool operator==(const SkullCrackPlan&)const=default;
};
inline std::optional<SkullCrackPlan> skull_crack(){
 const auto* r=skills::find_skill(329);
 if(!r||r->type!=14||!r->elite()||r->attribute!=51||r->weapon_requirement!=185||r->adrenaline!=220||
    r->activation!=.5f||r->aftercast!=0||r->recharge||r->bonus0!=15||r->bonus15!=15||
    r->caster_body_resource!=2077||r->target_body_resource!=576)return {};
 return SkullCrackPlan{};
}
struct SkullCrackContact {bool attempt_interrupt=false,apply_dazed=false;std::uint64_t duration=0;};
// Protection is deliberately not a predicate for the spell-only condition.
// A prevented interrupt does not erase a qualified Dazed application.
inline std::optional<SkullCrackContact> skull_crack_contact(const SkullCrackPlan& plan,
 const AttackActionFacts& action,AttackOutcome outcome,std::uint64_t now,bool conditions_allowed){
 const auto canonical=skull_crack();if(!canonical||*canonical!=plan||!valid_attack_outcome(outcome))return {};
 if(outcome!=AttackOutcome::hit)return SkullCrackContact{};
 const auto active=classify_interrupt_action(action);if(!active)return {};
 const bool dazed=active->spell&&conditions_allowed;
 if(dazed&&now>UINT64_MAX-plan.dazed_ms)return {};
 return SkullCrackContact{active->active,dazed,dazed?plan.dazed_ms:0};
}
}
