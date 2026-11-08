#pragma once
#include "gw_shield_stance.h"
#include "gw_attack_outcomes.h"
#include "gw_skillbar.h"
namespace fw::gw::skills::effects {
struct ShieldBashPlan {
 unsigned rank=0,energy=5;Time duration=5000,recharge=20000;
 unsigned additional_disable_ms=15000;
 bool operator==(const ShieldBashPlan&)const=default;
};
inline std::optional<ShieldBashPlan> shield_bash(unsigned rank){
 const auto* r=find_skill(363);
 if(rank>20||!r||r->type!=16||r->profession!=1||r->attribute!=17||r->energy_cost()!=5||
    r->adrenaline||r->activation||r->aftercast||r->recharge!=20||r->duration0!=5||r->duration15!=11||
    r->scale0!=15||r->scale15!=15||r->bonus0||r->bonus15||
    r->caster_body_resource!=621||r->target_body_resource!=2077)return {};
 return ShieldBashPlan{rank,5,1000ull*interpolate(5,11,rank)};
}
// Native14 is an actual attack skill; native20 PetAttack commands are not a
// pet's contact skill. Unknown IDs never become ordinary attacks implicitly.
inline std::optional<bool> shield_bash_attack_skill(unsigned skill){
 if(!skill)return false;const auto* row=find_skill(skill);if(!row)return {};return row->type==14;
}
struct ShieldBashProtection {unsigned instance=0;bool operator==(const ShieldBashProtection&)const=default;};
inline std::optional<ShieldBashProtection> shield_bash_protection(
 std::span<const Entry> entries,Time now,std::optional<ShieldEquipment> equipment,unsigned attack_skill){
 const auto attack=shield_bash_attack_skill(attack_skill);if(!attack)return {};
 if(!*attack)return ShieldBashProtection{};
 const Entry* active=nullptr;
 for(const auto& e:entries)if(e.skill==363&&e.applied<=now&&(!e.expires||now<*e.expires)){
  if(e.kind!=Category::other||!e.instance||!e.cause||!e.expires||!shield_bash(e.rank))return {};
  if(!active||std::tie(e.applied,e.instance)>std::tie(active->applied,active->instance))active=&e;
 }
 if(!active)return ShieldBashProtection{};
 if(!equipment||!equipment->revision)return {};
 return ShieldBashProtection{equipment->shield?active->instance:0};
}
inline std::optional<ShieldBashProtection> shield_bash_protection(
 const Registry& registry,Scope owner,Time now,std::optional<ShieldEquipment> equipment,unsigned attack_skill){
 if(!registry.matches(owner))return {};
 const auto attack=shield_bash_attack_skill(attack_skill);if(!attack)return {};
 const auto active=registry.newest_skill(owner,363,now);
 if(!active||!*attack)return ShieldBashProtection{};
 if(active->cause!=owner.actor||active->kind!=Category::other||!active->instance||!active->expires||!shield_bash(active->rank))return {};
 if(!equipment||!equipment->revision)return {};
 return ShieldBashProtection{equipment->shield?active->instance:0};
}
// A detached transaction, NOT an attack authorization. World must supply its
// exact server release/skill identity and attributed block (or the documented
// unblockable melee exception), then atomically preflight control/disable and
// commit this registry copy. No generic outcome or client packet can mint it.
struct ShieldBashContact {
 combat::weapon::ResolvedAttack attack;Slot skill;
 unsigned attributed_effect=0;bool unblockable_melee=false;
};
struct ShieldBashProposal {
 Registry registry;Entry removed;Scope attacker;Slot skill;std::uint64_t event=0;Time at=0;
 bool knockdown=false;unsigned additional_disable_ms=0;
};
inline std::optional<ShieldBashProposal> prepare_shield_bash_contact(const Registry& original,Scope owner,
 std::optional<ShieldEquipment> equipment,const ShieldBashContact& contact){
 const auto& a=contact.attack;
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!valid(owner)||!valid(a.attacker)||a.attacker.instance!=owner.instance||a.attacker.actor==owner.actor||
    a.target!=owner||!a.event||a.at<a.released||contact.skill.copy>65535||
    !combat::weapon::valid_attack_delivery(a.delivery)||a.delivery==combat::weapon::AttackDelivery::unknown)return {};
 const auto protection=shield_bash_protection(original,owner,a.at,equipment,contact.skill.skill);
 if(!protection||!protection->instance)return {};
 const bool melee=a.delivery==combat::weapon::AttackDelivery::melee;
 const auto* attack_row=find_skill(contact.skill.skill);
 // Native weapon requirements: melee axe/daggers/hammer/scythe/sword mask185,
 // ranged bow/spear mask66. Mixed/untyped requirements stay unsupported.
 const auto requirement=attack_row->weapon_requirement;
 if(!requirement||(melee?(requirement&~185u)||(requirement&185u)==0:
     (requirement&~66u)||(requirement&66u)==0))return {};
 // The bypass exception is only for a validated successful melee contact.
 // Miss/dodge/obstruction and another effect's block cannot spend this source.
 if(contact.unblockable_melee){
  if(!melee||a.outcome!=combat::weapon::AttackOutcome::hit||contact.attributed_effect||
     !a.before_damage||*a.before_damage!=a.at)return {};
 }else if(a.outcome!=combat::weapon::AttackOutcome::block||contact.attributed_effect!=protection->instance||a.before_damage)return {};
 auto next=original;const auto removed=next.remove(owner,protection->instance);if(!removed)return {};
 return ShieldBashProposal{std::move(next),*removed,a.attacker,contact.skill,a.event,a.at,melee,melee?15000u:0u};
}
}
