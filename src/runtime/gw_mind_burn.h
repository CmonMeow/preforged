#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include "gw_effect_registry.h"
#include <compare>
#include <set>

namespace fw::gw::skills::effects {
struct MindBurnPlan {
    unsigned skill=185,rank=0;double damage=15;
    std::uint64_t burning_ms=1000;float range=1248,radius=166;
    auto operator<=>(const MindBurnPlan&)const=default;
};
inline std::optional<MindBurnPlan> mind_burn(unsigned skill,unsigned rank=0){
    if(skill!=185||rank>20)return {};
    const auto* row=find_skill(skill);
    if(!row||row->type!=5||row->attribute!=10||row->scale0!=15||row->scale15!=60||
       row->bonus0!=1||row->bonus15!=10||row->encoded_energy!=5||row->overcast!=5||
       (row->special&5)!=5||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=5||row->pvp_skill!=3433)return {};
    return MindBurnPlan{skill,rank,damage_scale(15,60,rank),1000ull*interpolate(1,10,rank)};
}
struct MindBurnResult {
    bool accepted=false,energy_advantage=false,burning_applied=false;
    combat::Actor actor;combat::conditions::State conditions;
    std::vector<combat::Hit> hits;std::uint64_t burning_ms=0;
};
// Energy is the signed authoritative 300000-units/Energy snapshot AFTER caster
// payment/Overcast and regeneration. Compare to THIS recipient, not the anchor:
// Mind Burn's documented anomaly evaluates its condition independently per foe.
inline MindBurnResult mind_burn_target(const MindBurnPlan& plan,combat::Actor actor,
    combat::conditions::State conditions,unsigned level,unsigned caster,
    std::optional<std::int64_t> caster_energy,std::optional<std::int64_t> recipient_energy,
    std::optional<bool> burning_allowed,std::uint64_t now,const combat::ArmorSequence& armor_at_hit={},const combat::DamageProjection& projection={}){
    const auto canonical=mind_burn(plan.skill,plan.rank);constexpr auto limit=1000000ll*300000;
    if(!canonical||*canonical!=plan||!caster||caster>65535||!caster_energy||!recipient_energy||
       *caster_energy< -limit||*caster_energy>limit||*recipient_energy< -limit||*recipient_energy>limit||!actor.fire_armor)return {};
    actor.fire_armor=conditions.armor(*actor.fire_armor,now);
    MindBurnResult result;result.actor=actor;result.conditions=std::move(conditions);
    result.energy_advantage=*caster_energy>*recipient_energy;
    std::optional<combat::Hit> previous_damage;
    for(unsigned n=0;n<(result.energy_advantage?2u:1u);++n){
        if(armor_at_hit){const auto rating=armor_at_hit(previous_damage?&*previous_damage:nullptr);if(!rating)return {};
            result.actor.fire_armor=result.conditions.armor(*rating,now);}
        const auto step=combat::project_damage(result.actor,{combat::Kind::fire,plan.damage,level,plan.skill},projection);
        if(!step)return {};
        result.hits.push_back(step->incoming);previous_damage=step->projected;
        if(result.actor.health->current<=0)break;
    }
    if(result.energy_advantage&&result.actor.health->current>0){
        if(!burning_allowed)return {};
        if(*burning_allowed){
            if(!result.conditions.apply(combat::conditions::Kind::burning,caster,now,plan.burning_ms))return {};
            result.burning_applied=true;result.burning_ms=plan.burning_ms;
        }
    }
    result.accepted=true;return result;
}
struct MindBurnContact {
    Scope target;combat::Actor actor;combat::conditions::State conditions;
    std::optional<std::int64_t> energy;
    float x=0,y=0;unsigned plane=0;
    bool hostile=false;std::optional<bool> burning_allowed;
};
struct MindBurnRecipient {Scope target;MindBurnResult result;};
struct MindBurnAreaResult {bool accepted=false;std::vector<MindBurnRecipient> recipients;};
inline bool mind_burn_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
// Completion-time geometry and per-recipient proposals only. World owns range,
// LoS, live caster/target generation admission and atomic damage/condition/death
// publication. Unknown required data for any in-area foe rejects the whole batch.
inline MindBurnAreaResult mind_burn_area(const MindBurnPlan& plan,Scope caster,Scope selected,unsigned level,
    std::optional<std::int64_t> caster_energy,const std::vector<MindBurnContact>& contacts,std::uint64_t now){
    const auto canonical=mind_burn(plan.skill,plan.rank);
    if(!canonical||*canonical!=plan||!mind_burn_scope(caster)||!mind_burn_scope(selected)||
       caster.actor==selected.actor||caster.instance!=selected.instance)return {};
    const MindBurnContact* anchor=nullptr;
    for(const auto& contact:contacts)if(contact.target==selected){if(anchor)return {};anchor=&contact;}
    if(!anchor||!anchor->hostile||!std::isfinite(anchor->x)||!std::isfinite(anchor->y)||
       !anchor->actor.health||!combat::valid(*anchor->actor.health)||anchor->actor.health->current<=0||anchor->actor.id!=selected.actor||
       anchor->actor.generation!=selected.generation)return {};
    MindBurnAreaResult result;std::set<unsigned> seen;
    for(const auto& contact:contacts){
        if(!contact.hostile||contact.target.actor==caster.actor||contact.target.instance!=caster.instance||contact.plane!=anchor->plane)continue;
        if(!std::isfinite(contact.x)||!std::isfinite(contact.y))return {};
        if(std::hypot(contact.x-anchor->x,contact.y-anchor->y)>plan.radius)continue;
        if(!mind_burn_scope(contact.target)||contact.actor.id!=contact.target.actor||contact.actor.generation!=contact.target.generation||
           !seen.insert(contact.target.actor).second||!contact.actor.health||!combat::valid(*contact.actor.health))return {};
        if(contact.actor.health->current<=0)continue;
        auto proposed=mind_burn_target(plan,contact.actor,contact.conditions,level,caster.actor,caster_energy,
            contact.energy,contact.burning_allowed,now);
        if(!proposed.accepted)return {};
        result.recipients.push_back({contact.target,std::move(proposed)});
    }
    result.accepted=true;return result;
}
}
