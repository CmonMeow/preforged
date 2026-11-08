#pragma once
#include "gw_attack_interrupts.h"
#include "gw_hex_duration.h"
#include <compare>
namespace fw::gw::skills::effects {
// Initial damage samples an authoritative action at hex completion. It is
// separate from the target-only degeneration and never waits for a later use.
struct ActionDamageHexPlan {
 unsigned skill=0,rank=0,pips=0,damage=0;std::uint64_t duration=0;
 float range=1248,damage_radius=0;
 auto operator<=>(const ActionDamageHexPlan&)const=default;
};
inline std::optional<ActionDamageHexPlan> action_damage_hex(unsigned skill,unsigned rank=0){
 if(rank>20||(skill!=898&&skill!=899))return {};
 const auto* r=find_skill(skill);const bool overload=skill==898;
 if(!r||r->profession!=5||r->type!=4||r->attribute!=(overload?2u:1u)||r->special!=512||
    r->energy_cost()!=5||r->health_sacrifice||r->activation!=(overload?.25f:2.f)||r->aftercast!=.75f||r->recharge!=5||
    r->scale0!=1||r->scale15!=3||r->bonus0!=(overload?15u:10u)||r->bonus15!=(overload?75u:52u)||
    r->duration0!=5||r->duration15!=(overload?5u:10u)||r->caster_body_resource!=2077||
    r->target_body_resource!=(overload?960u:965u))return {};
 return ActionDamageHexPlan{skill,rank,interpolate(1,3,rank),interpolate(overload?15:10,overload?75:52,rank),
  std::uint64_t(interpolate(5,overload?5:10,rank))*1000,1248,overload?166.f:0.f};
}
struct ActionDamageHexReaction {ActionDamageHexPlan hex;unsigned initial_damage=0;float damage_radius=0;};
inline std::optional<ActionDamageHexReaction> prepare_action_damage_hex(const ActionDamageHexPlan& plan,
 const combat::weapon::AttackActionFacts& action){
 const auto canonical=action_damage_hex(plan.skill,plan.rank);if(!canonical||*canonical!=plan)return {};
 const auto classification=combat::weapon::classify_interrupt_action(action);if(!classification)return {};
 bool trigger=false;
 if(plan.skill==898){
  // Basic attacks have no skill slot. An explicit attack skill qualifies,
  // whether its authoritative action is represented as attack or skill.
  trigger=classification->active&&action.skill.has_value();
 }else if(action.kind==combat::weapon::AttackTargetAction::attack)trigger=true;
 else if(action.kind==combat::weapon::AttackTargetAction::skill&&action.skill){
  const auto* row=find_skill(action.skill->skill);trigger=row&&row->type==14;
 }
 return ActionDamageHexReaction{plan,trigger?plan.damage:0u,trigger?plan.damage_radius:0.f};
}
struct ActionDamageHexApplication {Registry registry;Applied change;Entry entry;ActionDamageHexReaction reaction;};
// A transaction-local registry proposal. Caller must settle target resources
// first and commit this together with initial damage/protection preflight.
inline std::optional<ActionDamageHexApplication> prepare_action_damage_hex_application(const Registry& registry,
 Scope caster,Scope target,unsigned effect,const ActionDamageHexPlan& plan,
 const combat::weapon::AttackActionFacts& action,Time now,std::optional<HexDuration> adjusted={}){
 const auto reaction=prepare_action_damage_hex(plan,action);if(!reaction)return {};
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!valid(caster)||!valid(target)||caster.instance!=target.instance||caster.actor==target.actor||
    !registry.matches(target)||!effect||registry.lookup(target,effect))return {};
 const auto duration=adjusted?adjusted->for_hex(plan.skill,plan.duration,caster.actor):std::optional(plan.duration);
 if(!duration||!*duration||now>UINT64_MAX-*duration)return {};
 const Entry entry{effect,caster.actor,plan.skill,plan.rank,Category::hex,now,now+*duration};
 auto proposed=registry;const auto change=proposed.apply(target,entry,Replace::same_skill);
 if(!change.accepted)return {};
 return ActionDamageHexApplication{std::move(proposed),change,entry,*reaction};
}

}

