#pragma once
#include "gw_dagger_chain.h"
#include "gw_dagger_identity_draft.h"
#include "gw_attack_outcomes.h"
#include "gw_conditions.h"
#include "gw_skill_effect_plan.h"
#include <optional>

namespace fw::gw::combat::weapon {
// Detached policy: no casting registration, payment, damage or packet delivery.
// These plans must join the normal weapon resolver rather than spell damage.
struct DaggerAttackPolicy {
    unsigned skill=0,rank=0,required_stage=0,result_stage=0;
    unsigned bonus=0,bleeding_ms=0,windup_ms=500;
    bool unblockable=false;
    bool operator==(const DaggerAttackPolicy&)const=default;
};
inline std::optional<DaggerAttackPolicy> dagger_attack_policy(unsigned skill,unsigned rank){
    const auto role=dagger_role(skill);if(rank>20||(role!=782&&role!=780))return {};
    const auto* row=skills::find_skill(skill);
    const bool lead=role==782,pvp=skill!=role;
    if(!row||row->campaign!=2||row->profession!=7||row->attribute!=29||row->type!=14||
       row->special!=(pvp?0x400000u:0u)||row->energy_cost()!=5||row->health_sacrifice||row->overcast!=99||row->adrenaline||
       row->activation!=.5f||row->aftercast!=0||row->recharge!=(lead?1u:pvp?6u:3u)||
       row->pvp_skill!=(lead?3433u:pvp?780u:3251u)||row->weapon_requirement!=8||row->target!=5||row->equip_type!=(pvp?0u:1u)||
       row->scale0!=(lead?5u:10u)||row->scale15!=(lead?20u:pvp?25u:35u)||row->bonus0||row->bonus15||
       row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
    return DaggerAttackPolicy{skill,rank,lead?0u:1u,lead?1u:2u,
        lead?0u:skills::effects::interpolate(10,pvp?25u:35u,rank),
        lead?1000u*skills::effects::interpolate(5,20,rank):0u,250,!lead};
}
inline bool canonical(const DaggerAttackPolicy& plan){return dagger_attack_policy(plan.skill,plan.rank)==plan;}
inline bool dagger_scope(skills::effects::Scope scope){
    return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;
}
// Call at both admission and contact with current equipment and chain. False
// specifies eligibility only; failure animation/payment/reset remains a caller
// decision until the failed-prerequisite retail consumers are reviewed.
inline bool dagger_attack_eligible(const DaggerAttackPolicy& plan,
        const skills::effects::DaggerChain& chain,skills::effects::Scope source,
        skills::effects::Scope target,unsigned item_type,bool caster_weapon,std::uint64_t now){
    return canonical(plan)&&dagger_scope(source)&&dagger_scope(target)&&
        source.instance==target.instance&&source.actor!=target.actor&&
        item_type==32&&!caster_weapon&&
        (!plan.required_stage||chain.current(target,now)==plan.required_stage);
}
struct DaggerContactProposal {
    skills::effects::Scope source,target;
    std::uint64_t event=0,at=0;
    skills::effects::DaggerChain before,after;
    unsigned bonus=0,bleeding_ms=0;
    bool publish_stage=false;
};
// QualifiedHit is supplied by the shared attack owner after accuracy/block and
// current lifetime/hostility/range checks. It is not authority to apply damage.
// Stage is proposed on a copy; World must preflight shared conditions, commit
// damage, reacquire living scopes, and deduplicate this exact event atomically.
inline std::optional<DaggerContactProposal> dagger_contact_proposal(
        const DaggerAttackPolicy& plan,const skills::effects::DaggerChain& chain,
        const QualifiedHit& contact,unsigned item_type,bool caster_weapon){
    if(!contact.event||contact.released>contact.at||contact.delivery!=AttackDelivery::melee||
       !dagger_attack_eligible(plan,chain,contact.attacker,contact.target,item_type,caster_weapon,contact.at))return {};
    auto next=chain;
    const bool changed=chain.current(contact.target,contact.at)!=plan.result_stage;
    if(!(plan.result_stage==1?next.lead(contact.target,contact.at):next.offhand(contact.target,contact.at)))return {};
    return DaggerContactProposal{contact.attacker,contact.target,contact.event,contact.at,chain,next,
        plan.bonus,plan.bleeding_ms,changed};
}
// Optimistic ledger check only: this does NOT supply exactly-once event authority.
inline bool dagger_chain_matches(const skills::effects::DaggerChain& current,
        const skills::effects::DaggerChain& expected){
    return current.target==expected.target&&current.stage==expected.stage&&current.expires==expected.expires;
}
}
