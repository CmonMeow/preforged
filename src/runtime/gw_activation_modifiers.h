#pragma once
#include "gw_effect_registry.h"
#include "gw_conditions.h"
#include "gw_aura_restoration.h"
#include "gw_deadly_paradox.h"
#include <algorithm>
#include <span>
namespace fw::gw::skills::effects {
inline double activation_multiplier(const SkillRecord& skill,std::span<const Entry> entries,
        const combat::conditions::State& conditions,Time now){
    // The documented Migraine attack-skill exception is deliberate. Dazed
    // affects spells, while Migraine also affects signets and other skills.
    if(!skill.id||skill.type==14)return 1.;
    bool migraine=false,conundrum=false,rust=false,paradox=false;
    for(const auto& e:entries)if(deadly_paradox_activates(skill,e,now))paradox=true;
    for(const auto& e:entries)if(e.kind==Category::hex&&e.applied<=now&&e.expires&&now<*e.expires){
        if(e.skill==53||e.skill==3183)migraine=true;
        if(e.skill==36)conundrum=true;
        if(e.skill==204)rust=true;
    }
    const bool spell=is_spell(skill);
    const double factor=(paradox?.75:1.)*(migraine?2.:1.)*(spell&&conundrum?2.:1.)*(skill.type==7&&rust?2.:1.)*
        (spell&&conditions.has(combat::conditions::Kind::dazed,now)?2.:1.);
    return std::clamp(factor,.75,2.5);
}
}
