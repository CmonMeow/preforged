#pragma once
#include "gw_dagger_attack_policy.h"
#include <array>

namespace fw::gw::combat::weapon {
struct DetachedDaggerFamilyPlan {
    unsigned skill=0,rank=0,bonus=0,duration_ms=0,energy=0,extra=0,strikes=1;
    bool operator==(const DetachedDaggerFamilyPlan&)const=default;
};
inline std::optional<DetachedDaggerFamilyPlan> detached_dagger_family_plan(unsigned skill,unsigned rank){
    if(rank>20)return {};
    unsigned attr=29,cost=5,recharge=0,special=0,low=0,high=0,b0=0,b15=0,pvp=3433;
    float area=0;
    const auto role=dagger_role(skill);const bool alias=skill!=role;
    switch(role){
    case 775:recharge=2;special=32768;low=20;high=45;pvp=3061;area=156;break;
    case 776:attr=35;cost=10;recharge=15;special=32768;low=10;high=20;b0=5;b15=20;break;
    case 777:recharge=12;special=32768;low=1;high=11;area=156;break;
    case 778:recharge=8;low=15;high=35;b0=5;b15=20;break;
    case 779:attr=35;cost=10;recharge=6;low=10;high=31;b0=5;b15=13;break;
    case 976:low=10;high=30;b0=15;b15=15;break;
    case 783:attr=35;cost=10;recharge=2;low=19;high=31;b0=15;b15=75;break;
    default:return {};
    }
    if(alias){if(role!=775)return {};recharge=6;special|=0x400000u;pvp=775;}
    const auto* row=skills::find_skill(skill);
    if(!row||row->campaign!=2||row->profession!=7||row->attribute!=attr||row->type!=14||
       row->special!=special||row->energy_cost()!=cost||row->adrenaline||row->health_sacrifice||row->overcast!=99||
       row->activation!=0||row->aftercast!=0||row->recharge!=recharge||row->pvp_skill!=pvp||
       row->weapon_requirement!=8||row->target!=5||row->equip_type!=(alias?0u:1u)||row->scale0!=low||row->scale15!=high||
       row->bonus0!=b0||row->bonus15!=b15||row->aoe_range!=area||
       row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
    const auto scale=[](unsigned a,unsigned b,unsigned rank){return skills::effects::interpolate(a,b,rank);};
    return DetachedDaggerFamilyPlan{skill,rank,scale(low,high,rank),
        (role==776||role==778)?1000u*scale(5,20,rank):0u,
        role==779?scale(5,13,rank):0u,role==783?scale(15,75,rank):0u,special?2u:1u};
}
struct DetachedDaggerContactFacts {
    std::optional<bool> target_hexed,target_knocked_down,target_has_adjacent_ally;
    std::optional<Health> target_health;
};
struct DetachedDaggerConditionRequest {conditions::Kind kind=conditions::Kind::bleeding;unsigned base_ms=0;};
struct DetachedDaggerFamilyEffect {
    QualifiedHit contact;
    unsigned bonus=0,energy_gain=0,knockdown_ms=0,adjacent_damage=0,result_stage=0;
    std::array<DetachedDaggerConditionRequest,2> conditions{};
    unsigned condition_count=0;
    // Dual requests are PER STRIKE. The World dual owner supplies two unique
    // actual contact events; this helper neither generates nor merges them.
    unsigned strikes=1;
};
// Pure request only. For duals the caller passes the combo's retained pre-hit
// stage2 snapshot for EACH independently qualified strike. World must own that
// snapshot/group, current scopes and once-only stage/payment; this function
// cannot prove a caller-supplied snapshot or pair unrelated contacts safely.
inline std::optional<DetachedDaggerFamilyEffect> detached_dagger_family_effect(
        const DetachedDaggerFamilyPlan& plan,const skills::effects::DaggerChain& admitted_chain,
        const QualifiedHit& hit,const DetachedDaggerContactFacts& facts,unsigned item_type=32,bool caster_weapon=false){
    if(detached_dagger_family_plan(plan.skill,plan.rank)!=plan||!dagger_scope(hit.attacker)||!dagger_scope(hit.target)||
       hit.attacker.instance!=hit.target.instance||hit.attacker.actor==hit.target.actor||!hit.event||hit.released>hit.at||
       hit.delivery!=AttackDelivery::melee||item_type!=32||caster_weapon)return {};
    if((plan.strikes==2||dagger_role(plan.skill)==976)&&admitted_chain.current(hit.target,hit.at)!=2)return {};
    if(dagger_role(plan.skill)==778&&(!facts.target_knocked_down||!*facts.target_knocked_down))return {};
    if(dagger_role(plan.skill)==779&&!facts.target_hexed)return {};
    if(dagger_role(plan.skill)==777&&!facts.target_has_adjacent_ally)return {};
    if(dagger_role(plan.skill)==783&&(!facts.target_health||!combat::valid(*facts.target_health)||facts.target_health->current<=0))return {};
    DetachedDaggerFamilyEffect result;result.contact=hit;result.bonus=plan.bonus;result.strikes=plan.strikes;
    result.result_stage=plan.strikes==2?3u:(dagger_role(plan.skill)==778||dagger_role(plan.skill)==976)?2u:1u;
    if(dagger_role(plan.skill)==775)result.adjacent_damage=plan.bonus;
    if(dagger_role(plan.skill)==776){result.conditions={DetachedDaggerConditionRequest{conditions::Kind::bleeding,plan.duration_ms},
        DetachedDaggerConditionRequest{conditions::Kind::deep_wound,plan.duration_ms}};result.condition_count=2;}
    if(dagger_role(plan.skill)==777&&!*facts.target_has_adjacent_ally)result.knockdown_ms=2000;
    if(dagger_role(plan.skill)==778){result.conditions[0]={conditions::Kind::poison,plan.duration_ms};result.condition_count=1;}
    if(dagger_role(plan.skill)==779&&*facts.target_hexed)result.energy_gain=plan.energy;
    if(dagger_role(plan.skill)==783&&facts.target_health->current*10>double(facts.target_health->maximum)*9)result.bonus+=plan.extra;
    return result;
}
// Sequential copied-state condition projection. Duration/immunity/reactions
// remain a PURE caller callback; Bleeding then Deep Wound order is preserved.
template<class State,class Application>
std::optional<State> detached_dagger_project_conditions(const DetachedDaggerFamilyEffect& effect,State state,Application&& apply){
    if(effect.condition_count>effect.conditions.size())return {};
    for(unsigned i=0;i<effect.condition_count;++i){const auto request=effect.conditions[i];
        if(!apply(state,request.kind,effect.contact.attacker.actor,effect.contact.at,request.base_ms))return {};
    }
    return state;
}
}
