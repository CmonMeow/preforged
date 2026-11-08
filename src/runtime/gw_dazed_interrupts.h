#pragma once
#include "gw_attack_interrupts.h"
#include "gw_conditions.h"
#include "gw_attack_outcomes.h"
namespace fw::gw::combat::weapon {
struct ConcussionPlan {unsigned rank=0,damage=1,energy=25;std::uint64_t dazed_ms=5000,windup=500;auto operator<=>(const ConcussionPlan&)const=default;};
inline std::optional<ConcussionPlan> concussion_shot(unsigned rank){
 const auto* r=skills::find_skill(408);
 if(rank>20||!r||r->type!=14||r->attribute!=25||r->energy_cost()!=25||r->activation!=.5f||r->aftercast!=1.5f||r->recharge!=5||r->scale0!=1||r->scale15!=16||r->bonus0!=5||r->bonus15!=20)return {};
 return ConcussionPlan{rank,1+rank,25,1000ull*(5+rank),500};
}
struct ConcussionContact {unsigned damage=0;bool apply_dazed=false;std::uint64_t duration=0;};
// There is no direct interrupt here: local Concussion_Shot notes establish
// that retained NEW Dazed causes it. A condition redirect/immunity must not
// become an interrupt,and an existing Dazed timer must not be renewed.
inline std::optional<ConcussionContact> concussion_contact(const ConcussionPlan& plan,const AttackActionFacts& action,const conditions::State& before,std::uint64_t now,bool can_receive_dazed){
 const auto canonical=concussion_shot(plan.rank);if(!canonical||*canonical!=plan)return {};
 const auto c=classify_interrupt_action(action);if(!c)return {};
 const bool apply=c->spell&&can_receive_dazed&&!before.has(conditions::Kind::dazed,now);
 if(apply&&now>UINT64_MAX-plan.dazed_ms)return {};
 return ConcussionContact{plan.damage,apply,apply?plan.dazed_ms:0};
}
// Shared condition transactions call this on their committed BEFORE/AFTER
// states. A proposed Dazed redirected elsewhere is not a retained condition.
// This covers first application only. Generic renewal interruption is not
// established by capture; Concussion-specific nonrenewal is a separate rule.
inline std::optional<bool> newly_dazed_interrupt(const conditions::State& before,const conditions::State& after,const AttackActionFacts& action,std::uint64_t now){
 if(before.has(conditions::Kind::dazed,now)||!after.has(conditions::Kind::dazed,now))return false;
 const auto c=classify_interrupt_action(action);return c?std::optional(c->spell):std::nullopt;
}
inline std::optional<bool> dazed_hit_interrupt(const conditions::State& target,const AttackActionFacts& action,AttackOutcome outcome,std::uint64_t now){
 if(outcome!=AttackOutcome::hit||!target.has(conditions::Kind::dazed,now))return false;
 const auto c=classify_interrupt_action(action);return c?std::optional(c->spell):std::nullopt;
}
}
