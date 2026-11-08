#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_skillbar.h"
#include "gw_aura_restoration.h"
namespace fw::gw::combat::weapon {
struct SavageSlashPlan {unsigned rank=0,bonus=0,weapon_type=27;std::uint64_t windup=500;auto operator<=>(const SavageSlashPlan&)const=default;};
inline std::optional<SavageSlashPlan> savage_slash(unsigned rank){
 const auto* r=skills::find_skill(390);
 if(rank>20||!r||r->type!=14||r->attribute!=20||r->energy_cost()!=5||r->recharge!=15||r->activation!=.5f||r->scale0!=1||r->scale15!=40)return {};
 return SavageSlashPlan{rank,skills::effects::interpolate(1,40,rank)};
}
struct SavageShotPlan {unsigned rank=0,bonus=0,weapon_type=5;std::uint64_t windup=500;auto operator<=>(const SavageShotPlan&)const=default;};
inline std::optional<SavageShotPlan> savage_shot(unsigned rank){
 const auto* r=skills::find_skill(426);
 if(rank>20||!r||r->type!=14||r->attribute!=25||r->energy_cost()!=10||r->recharge!=5||r->activation!=.5f||r->aftercast!=1.5f||r->scale0!=13||r->scale15!=28)return {};
 return SavageShotPlan{rank,13+rank};
}
inline std::optional<SavageShotPlan> punishing_shot(unsigned rank){
 const auto* r=skills::find_skill(409);
 if(rank>20||!r||r->type!=14||r->attribute!=25||r->energy_cost()!=10||r->recharge!=5||r->activation!=.5f||r->aftercast!=1.5f||r->scale0!=10||r->scale15!=20)return {};
 return SavageShotPlan{rank,skills::effects::interpolate(10,20,rank)};
}
enum class AttackTargetAction {idle,attack,skill};
struct AttackActionFacts {AttackTargetAction kind=AttackTargetAction::idle;std::optional<skills::Slot> skill;};
struct SavageSlashContact {unsigned bonus=0;bool attempt_interrupt=false;};
// Trusted impact-time action identity,not a client-selected target flag.
struct InterruptActionClass {bool active=false,spell=false;};
inline std::optional<InterruptActionClass> classify_interrupt_action(const AttackActionFacts& action){
 if(action.kind==AttackTargetAction::idle)return !action.skill?std::optional<InterruptActionClass>{{false,false}}:std::nullopt;
 if(action.kind==AttackTargetAction::attack){
  if(action.skill){const auto* row=skills::find_skill(action.skill->skill);if(!row||!row->id||row->type!=14||action.skill->copy>65535)return {};}
  return InterruptActionClass{true,false};
 }
 if(action.kind!=AttackTargetAction::skill||!action.skill||!action.skill->skill||action.skill->copy>65535)return {};
 const auto* row=skills::find_skill(action.skill->skill);if(!row)return {};
 return InterruptActionClass{true,skills::effects::is_spell(*row)};
}
// Unknown active skills cannot be silently treated as non-spells. Protection
// is applied later;it does not remove the spell-only damage bonus.
inline std::optional<SavageSlashContact> savage_slash_contact(const SavageSlashPlan& plan,const AttackActionFacts& action){
 const auto canonical=savage_slash(plan.rank);if(!canonical||*canonical!=plan)return {};
 const auto c=classify_interrupt_action(action);if(!c)return {};
 return SavageSlashContact{c->spell?plan.bonus:0,c->active};
}
inline std::optional<SavageSlashContact> savage_shot_contact(const SavageShotPlan& plan,const AttackActionFacts& action){
 const auto canonical=savage_shot(plan.rank);if(!canonical||*canonical!=plan)return {};
 const auto c=classify_interrupt_action(action);if(!c)return {};
 return SavageSlashContact{c->spell?plan.bonus:0,c->active};
}
}
