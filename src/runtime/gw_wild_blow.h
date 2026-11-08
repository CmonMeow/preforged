#pragma once
#include "gw_weapon_attack.h"
#include "gw_attack_outcomes.h"
#include "gw_effect_registry.h"
namespace fw::gw::combat::weapon {
struct WildBlowPlan {
 unsigned skill=321,energy=5,recharge_ms=8000;
 bool unblockable=true,critical=true,lose_all_adrenaline=true;
 bool operator==(const WildBlowPlan&)const=default;
};
inline std::optional<WildBlowPlan> wild_blow(){
 const auto* r=skills::find_skill(321);
 if(!r||r->type!=14||r->attribute!=51||r->weapon_requirement!=185||r->energy_cost()!=5||
    r->adrenaline||r->activation!=0||r->aftercast!=0||r->recharge!=8||
    r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 return WildBlowPlan{};
}
// Uses shared weapon/Weakness/armor/critical-immunity authority. Unblockable
// bypasses blocking,not Blindness or any other established miss source.
inline std::optional<Result> wild_blow_damage(const WildBlowPlan& plan,Strike strike){
 const auto canonical=wild_blow();if(!canonical||*canonical!=plan||strike.caster_weapon)return {};
 strike.critical=true;return resolve(strike);
}
struct WildBlowContact {std::optional<unsigned> remove_stance;};
// Exact owned stance identity is retained for the eventual atomic World commit.
// An absent or naturally expired stance is a valid no-removal result.
inline std::optional<WildBlowContact> wild_blow_contact(const WildBlowPlan& plan,
 AttackScope target,const std::optional<skills::effects::Entry>& stance,AttackOutcome outcome,std::uint64_t now){
 const auto canonical=wild_blow();if(!canonical||*canonical!=plan||!target.instance||!target.actor||!target.generation||!valid_attack_outcome(outcome))return {};
 if(outcome!=AttackOutcome::hit||!stance)return WildBlowContact{};
 if(!stance->instance||!stance->cause||stance->kind!=skills::effects::Category::stance||stance->applied>now)return {};
 const auto* row=skills::find_skill(stance->skill);if(!row||row->type!=3)return {};
 if(stance->expires&&now>=*stance->expires)return WildBlowContact{};
 return WildBlowContact{stance->instance};
}
}
