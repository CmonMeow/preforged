#pragma once
#include "gw_dagger_transaction_detached.h"
#include "gw_weapon_attack_skills.h"

namespace fw::gw::combat::weapon {
// Adapter for eventual attack_skill() integration. It is intentionally NOT
// accepted by the current production valid(AttackPlan) dispatcher.
inline std::optional<AttackPlan> detached_dagger_attack_plan(unsigned skill,unsigned rank,
        unsigned strength=0,unsigned copy=0){
    const auto policy=dagger_attack_policy(skill,rank);
    if(!policy||strength>20||copy>65535)return {};
    AttackPlan plan{skill,rank,policy->bonus,0,policy->windup_ms};
    plan.required_type=32;plan.strength=strength;plan.copy=copy;return plan;
}
inline bool detached_dagger_plan_valid(const AttackPlan& plan){
    return detached_dagger_attack_plan(plan.skill,plan.rank,plan.strength,plan.copy)==plan;
}
// Same shared base-weapon resolver and separate bonus/Strength convention as
// resolve_attack_skill. Block, critical sampling and equipment authority are
// owned by the caller. A QualifiedHit and a missed Strike are inconsistent.
inline std::optional<SkillHit> detached_dagger_skill_hit(const AttackPlan& plan,const Strike& strike){
    if(!detached_dagger_plan_valid(plan)||strike.item_type!=32||strike.caster_weapon)return {};
    const auto base=resolve(strike);if(!base||base->missed)return {};
    const double penetration=strike.location_armor>0?
        std::exp2(strike.location_armor*plan.strength*.01/40.):1.;
    const auto damage=base->damage*penetration+plan.bonus;
    if(!std::isfinite(damage)||damage>1000000)return {};
    return SkillHit{false,damage,0,0,false,base->critical,base->critical_energy};
}
struct DetachedDaggerEffects {
    SkillHit weapon;
    DetachedDaggerTransaction transaction;
};
// Complete copied-state direct damage + shared-condition projection. Actual
// World reactions/protections that require owned effect retirement must still
// use the callback composition helper, not this Actor-only convenience path.
// The caller establishes hostility, accuracy/block and current critical facts.
// weapon_kind is actual equipped damage type, independent of armor bypass:
// armor was already resolved once by the weapon resolver above.
template<class ConditionMutation>
std::optional<DetachedDaggerEffects> prepare_detached_dagger_effects(
        const AttackPlan& plan,const skills::effects::DaggerChain& chain,
        const QualifiedHit& qualified,const Strike& strike,Kind weapon_kind,
        Actor target,conditions::State conditions,ConditionMutation&& condition_mutation){
    if(weapon_kind!=Kind::slashing&&weapon_kind!=Kind::piercing&&weapon_kind!=Kind::earth&&
       weapon_kind!=Kind::fire&&weapon_kind!=Kind::cold&&weapon_kind!=Kind::lightning)return {};
    const auto policy=dagger_attack_policy(plan.skill,plan.rank);
    const auto weapon=detached_dagger_skill_hit(plan,strike);
    if(!policy||!weapon)return {};
    const auto damage=[&](Actor& copied,unsigned bonus)->std::optional<Hit>{
        if(bonus!=plan.bonus)return {};
        Damage incoming{weapon_kind,weapon->damage,strike.level,plan.skill,0,true};
        const auto hit=apply(copied,incoming,true);
        return hit.reason==Reason::accepted?std::optional<Hit>{hit}:std::nullopt;
    };
    const auto transaction=prepare_detached_dagger_transaction(*policy,chain,qualified,
        strike.item_type,strike.caster_weapon,std::move(target),std::move(conditions),damage,
        std::forward<ConditionMutation>(condition_mutation));
    if(!transaction)return {};
    return DetachedDaggerEffects{*weapon,*transaction};
}
}
