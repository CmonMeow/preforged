#pragma once
#include "gw_attack_interrupts.h"
#include "gw_effect_registry.h"
#include "gw_attack_outcomes.h"
#include <cmath>
#include <set>
#include <span>
namespace fw::gw::combat::weapon {
struct DistractingBlowPlan {
 unsigned skill=325,energy=5,recharge_ms=10000;
 std::uint64_t windup=500;double adjacent=166.;
 bool zero_weapon_damage=true;
 bool operator==(const DistractingBlowPlan&)const=default;
};
inline std::optional<DistractingBlowPlan> distracting_blow(){
 const auto* r=skills::find_skill(325);
 if(!r||r->type!=14||r->attribute!=51||r->weapon_requirement!=185||r->energy_cost()!=5||r->adrenaline||
    r->activation!=.5f||r->aftercast!=0||r->recharge!=10||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 return DistractingBlowPlan{};
}
struct BlowRecipient {skills::effects::Scope scope;double x=0,y=0;bool eligible=false;AttackActionFacts action;};
struct BlowInterrupt {skills::effects::Scope scope;bool attempt=false;};
// Pure spatial/action preflight only. World owns hostility,life,layer and settled
// positions behind eligible. This does not decide whether a missed/blocked main
// swipe suppresses secondary interruptions,nor create secondary weapon hits.
inline std::optional<std::vector<BlowInterrupt>> distracting_blow_recipients(const DistractingBlowPlan& plan,
 skills::effects::Scope source,skills::effects::Scope primary,std::span<const BlowRecipient> candidates){
 const auto canonical=distracting_blow();
 if(!canonical||*canonical!=plan||!source.instance||!source.actor||source.actor>65535||!source.generation||
    primary.instance!=source.instance||!primary.actor||primary.actor>65535||!primary.generation||primary.actor==source.actor||candidates.size()>4096)return {};
 const BlowRecipient* center=nullptr;std::set<unsigned> seen;
 for(const auto& c:candidates){
  if(c.scope.instance!=source.instance||!c.scope.actor||c.scope.actor>65535||!c.scope.generation||
     !seen.insert(c.scope.actor).second||!std::isfinite(c.x)||!std::isfinite(c.y))return {};
  if(c.scope==primary)center=&c;
 }
 if(!center||!center->eligible)return {};
 std::vector<BlowInterrupt> selected;
 const auto append=[&](const BlowRecipient& c){const auto action=classify_interrupt_action(c.action);if(!action)return false;selected.push_back({c.scope,action->active});return true;};
 if(!append(*center))return {};
 for(const auto& c:candidates){
  if(c.scope.actor==source.actor||c.scope==primary||!c.eligible||std::hypot(c.x-center->x,c.y-center->y)>plan.adjacent)continue;
  if(!append(c))return {};
 }
 return selected;
}
// Validate attempted weapon recipients separately from area interrupts.
// World supplies scythe selection here,then resolves each contact's accuracy
// and block afterwards. These are not a list of landed hits;the helper never
// turns every adjacent interruption into an attack hit.
// It intentionally makes no claim about primary-miss -> area-effect gating.
inline bool distracting_blow_contacts(const DistractingBlowPlan& plan,
 skills::effects::Scope source,skills::effects::Scope primary,unsigned item_type,
 std::span<const BlowRecipient> candidates,std::span<const skills::effects::Scope> contacts){
 const auto area=distracting_blow_recipients(plan,source,primary,candidates);
 if(!area||weapon_delivery(item_type,false)!=AttackDelivery::melee||contacts.empty()||
    contacts.front()!=primary||contacts.size()>(item_type==35?3u:1u))return false;
 std::set<unsigned> seen;
 for(const auto& contact:contacts){
  if(!seen.insert(contact.actor).second||std::none_of(area->begin(),area->end(),[&](const auto& recipient){return recipient.scope==contact;}))return false;
 }
 return true;
}
}
