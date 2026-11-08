#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_attack_outcomes.h"
#include "gw_cooldown_bar_authority.h"
namespace fw::gw::skills::effects {
// Pinned2022 policy. The newer capture's12s recharge/1s duration is NOT
// substituted for this version's20s recharge/5..11s duration.
struct BullsChargePlan {
 unsigned rank=0,energy=5;Time duration=5000,recharge=20000;float movement=1.33f;
 bool operator==(const BullsChargePlan&)const=default;
};
inline std::optional<BullsChargePlan> bulls_charge(unsigned rank){
 const auto* r=find_skill(379);
 if(rank>20||!r||r->profession!=1||r->attribute!=17||r->type!=3||!r->elite()||
    r->energy_cost()!=5||r->adrenaline||r->activation||r->aftercast||r->recharge!=20||
    r->duration0!=5||r->duration15!=11||r->scale0!=33||r->scale15!=33||r->bonus0||r->bonus15||
    r->caster_body_resource!=638||r->target_body_resource!=2077)return {};
 return BullsChargePlan{rank,5,1000ull*interpolate(5,11,rank)};
}
inline std::optional<Entry> active_bulls_charge(const Registry& registry,Scope owner,Time now){
 if(!registry.matches(owner))return {};
 const auto e=registry.newest_skill(owner,379,now);
 return e&&e->instance&&e->cause==owner.actor&&e->kind==Category::stance&&e->expires&&bulls_charge(e->rank)?e:std::nullopt;
}
// Detached proposal only. Caller must provide authoritative movement at contact,
// deduplicate the terminal event, then preflight immunity/control and publish KD.
// Zero damage is still a successful hit; damage amount is not an eligibility test.
inline std::optional<bool> bulls_charge_knockdown(const Registry& registry,Scope owner,
 const combat::weapon::ResolvedAttack& attack,bool target_moving){
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!registry.matches(owner)||!valid(owner)||!valid(attack.target)||attack.attacker!=owner||
    attack.target.instance!=owner.instance||attack.target.actor==owner.actor||!attack.event||
    attack.at<attack.released||!combat::weapon::valid_attack_outcome(attack.outcome)||!combat::weapon::valid_attack_delivery(attack.delivery)||
    attack.delivery==combat::weapon::AttackDelivery::unknown)return {};
 const auto active=registry.newest_skill(owner,379,attack.at);
 if(active&&!active_bulls_charge(registry,owner,attack.at))return {};
 return bool(active_bulls_charge(registry,owner,attack.at))&&target_moving&&
   attack.delivery==combat::weapon::AttackDelivery::melee&&attack.outcome==combat::weapon::AttackOutcome::hit;
}
struct BullsChargeUseProposal {Registry registry;std::optional<Entry> ended;};
inline std::optional<BullsChargeUseProposal> prepare_bulls_charge_accepted_use(const Registry& original,
 Scope owner,Slot used,const CooldownBarSnapshot& bar,Time now){
 if(!original.matches(owner)||!cooldown_slot(used)||bar.published.owner!=owner||!bar.published.order||
    !bar.revision||bar.published.at>now||std::find(bar.bar.begin(),bar.bar.end(),used)==bar.bar.end())return {};
 auto next=original;const auto e=active_bulls_charge(original,owner,now);
 if(e&&!next.remove(owner,e->instance))return {};
 return BullsChargeUseProposal{std::move(next),e};
}
}
