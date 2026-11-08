#pragma once
#include "gw_dazed_interrupts.h"
#include "gw_precision_shot.h"
#include "gw_heal_effects.h"
namespace fw::gw::combat::weapon {
// Only reviewed easily-interrupted actions are admitted. Ordinary attack
// animations do not make all attack skills vulnerable to successful hits.
inline std::optional<bool> reviewed_hit_interrupt(const conditions::State& target,const AttackActionFacts& action,AttackOutcome outcome,std::uint64_t now){
 if(outcome!=AttackOutcome::hit)return false;
 if(action.skill&&action.skill->skill==400){
  if(!precision_shot(0))return {};
  const auto classified=classify_interrupt_action(action);
  return classified?std::optional(classified->active):std::nullopt;
 }
 if(action.skill&&action.skill->skill==959){
  if(!skills::effects::direct_heal(959))return {};
  const auto classified=classify_interrupt_action(action);
  return classified?std::optional(classified->active&&classified->spell):std::nullopt;
 }
 return dazed_hit_interrupt(target,action,outcome,now);
}
}
