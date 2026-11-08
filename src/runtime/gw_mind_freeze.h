#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct MindFreezePlan {
    unsigned skill=209,rank=0;double damage=10;std::uint64_t duration=1000;
    float range=1248,multiplier=.1f;
    auto operator<=>(const MindFreezePlan&)const=default;
};
inline std::optional<MindFreezePlan> mind_freeze(unsigned skill,unsigned rank=0){
    if((skill!=209&&skill!=2803)||rank>20)return {};
    const auto* row=find_skill(skill);const unsigned high=skill==209?60:40;
    if(!row||row->type!=4||row->attribute!=11||row->scale0!=10||row->scale15!=high||
       row->bonus0!=90||row->bonus15!=90||row->duration0!=1||row->duration15!=5||
       row->overcast!=5||row->encoded_energy!=(skill==209?5u:10u)||row->pvp_skill!=(skill==209?2803u:209u))return {};
    return MindFreezePlan{skill,rank,damage_scale(10,high,rank),1000ull*interpolate(1,5,rank)};
}
struct MindFreezeResult {
    bool accepted=false,energy_advantage=false;combat::Actor actor;
    std::vector<combat::Hit> hits;std::uint64_t snare_ms=0;
};
// Authoritative signed300000-units/Energy values must already include payment,
// debt,drains and fractional regeneration. Hex admission and publication belong
// to World; this pure proposal never mutates a live actor or creates a timer.
inline MindFreezeResult mind_freeze_target(const MindFreezePlan& plan,combat::Actor actor,
        const combat::conditions::State& conditions,unsigned level,std::optional<std::int64_t> caster_energy,
        std::optional<std::int64_t> target_energy,std::uint64_t now,const combat::ArmorSequence& armor_at_hit={},const combat::DamageProjection& projection={}){
    const auto canonical=mind_freeze(plan.skill,plan.rank);constexpr auto limit=1000000ll*300000;
    if(!canonical||*canonical!=plan||!caster_energy||!target_energy||*caster_energy< -limit||*caster_energy>limit||
       *target_energy< -limit||*target_energy>limit||!actor.cold_armor)return {};
    actor.cold_armor=conditions.armor(*actor.cold_armor,now);
    MindFreezeResult result;result.actor=actor;result.energy_advantage=*caster_energy>*target_energy;
    std::optional<combat::Hit> previous_damage;
    for(unsigned n=0;n<(result.energy_advantage?2u:1u);++n){
        if(armor_at_hit){const auto rating=armor_at_hit(previous_damage?&*previous_damage:nullptr);if(!rating)return {};
            result.actor.cold_armor=conditions.armor(*rating,now);}
        const auto step=combat::project_damage(result.actor,{combat::Kind::cold,plan.damage,level,plan.skill},projection);
        if(!step)return {};
        result.hits.push_back(step->incoming);previous_damage=step->projected;
        if(result.actor.health->current<=0)break;
    }
    if(result.energy_advantage&&result.actor.health->current>0){
        if(now>UINT64_MAX-plan.duration)return {};
        result.snare_ms=plan.duration;
    }
    result.accepted=true;return result;
}
}
