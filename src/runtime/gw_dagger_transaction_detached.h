#pragma once
#include "gw_dagger_attack_policy.h"
#include <utility>

namespace fw::gw::combat::weapon {
struct DetachedDaggerTransaction {
    DaggerContactProposal contact;
    Actor target;
    conditions::State conditions;
    Hit damage;
    skills::effects::DaggerChain chain;
    bool publish_stage=false,condition_applied=false;
};
// Pure speculative composition over copied state. Both callbacks must operate
// ONLY on their arguments: no captured World mutation or publication. The
// damage callback owns the shared weapon/protection resolver and returns its
// accepted Hit; bonus is separate from weapon damage. The condition callback
// owns duration modifiers, immunity and pure application/reaction projection.
// It receives base duration, never an instruction to bypass shared authority.
// This helper owns neither payment nor event replay, and cannot commit World.
template<class DamageMutation,class ConditionMutation>
std::optional<DetachedDaggerTransaction> prepare_detached_dagger_transaction(
        const DaggerAttackPolicy& plan,const skills::effects::DaggerChain& chain,
        const QualifiedHit& qualified,unsigned item_type,bool caster_weapon,
        Actor target,conditions::State conditions,
        DamageMutation&& damage_mutation,ConditionMutation&& condition_mutation){
    const auto proposal=dagger_contact_proposal(plan,chain,qualified,item_type,caster_weapon);
    if(!proposal||target.id!=qualified.target.actor||target.generation!=qualified.target.generation||
       !target.damageable||!target.health||!combat::valid(*target.health)||target.health->current<=0)return {};
    const auto before=*target.health;
    auto damage=std::forward<DamageMutation>(damage_mutation)(target,proposal->bonus);
    if(!damage||damage->reason!=Reason::accepted||damage->target!=qualified.target.actor||
       damage->generation!=qualified.target.generation||damage->skill!=plan.skill||
       !valid_damage_health(*damage)||damage->before.current!=before.current||damage->before.maximum!=before.maximum||
       damage->applied!=std::min(damage->damage,before.current)||!std::isfinite(damage->damage)||damage->damage<0||damage->damage>16000000||
       target.id!=qualified.target.actor||target.generation!=qualified.target.generation||
       !target.health||target.health->current!=damage->after.current||target.health->maximum!=damage->after.maximum)return {};
    bool applied=false;
    if(target.health->current>0&&proposal->bleeding_ms){
        if(!std::forward<ConditionMutation>(condition_mutation)(target,conditions,
                qualified.attacker.actor,qualified.at,proposal->bleeding_ms))return {};
        applied=true;
    }
    if(target.id!=qualified.target.actor||target.generation!=qualified.target.generation||
       !target.health||!combat::valid(*target.health))return {};
    // Survivor-only chain is the current private-server policy. A successful
    // lethal retail dagger contrast remains absent; this is not retail proof.
    const bool survives=target.health->current>0;
    return DetachedDaggerTransaction{*proposal,std::move(target),std::move(conditions),*damage,
        survives?proposal->after:chain,survives&&proposal->publish_stage,applied};
}
}
