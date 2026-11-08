#pragma once
#include "gw_effect_registry.h"
#include <cmath>
#include <span>

namespace fw::gw::skills::effects {
struct ControlHexPlan {
    unsigned skill=0,rank=0;
    Time duration=0;
    float range=1248.f,radius=0;unsigned miss_percent=0;
};
inline std::optional<ControlHexPlan> control_hex(unsigned skill,unsigned rank=0){
    if(rank>20||(skill!=35&&skill!=56&&skill!=235&&skill!=883))return {};
    const auto* row=find_skill(skill);
    if(!row||row->type!=4||row->attribute!=(skill==883?7u:skill==235?11u:skill==35?2u:1u))return {};
    if(skill==883){
        if(row->campaign!=3||row->profession!=4||row->energy_cost()!=10||row->activation!=1.f||
           row->aftercast!=.75f||row->recharge!=20||row->duration0!=5||row->duration15!=20||
           row->caster_body_resource!=1617||row->target_body_resource!=2077)return {};
        return ControlHexPlan{883,rank,1000ull*(5+rank),1248.f,252.f};
    }
    if(skill==235&&(row->scale0!=50||row->scale15!=50||row->duration0!=4||row->duration15!=10))return {};
    const auto duration=static_cast<Time>(std::lround(row->duration0+(double(row->duration15)-row->duration0)*rank/15.))*1000;
    return ControlHexPlan{skill,rank,duration,1248.f,(skill==56||skill==235)?166.f:0.f,skill==235?50u:0u};
}
inline bool active_control_hex(std::span<const Entry> entries,unsigned skill,Time now){
    for(const auto& entry:entries)if(entry.skill==skill&&entry.kind==Category::hex&&
        entry.applied<=now&&entry.expires&&now<*entry.expires)return true;
    return false;
}
inline bool signet_forbidden(const SkillRecord& skill,std::span<const Entry> entries,Time now){
    return skill.type==7&&active_control_hex(entries,35,now);
}
// Admission restriction,not a recharge timer or an interrupt. Existing actions
// keep their tokens;normal expiry/removal restores subsequent use.
inline bool vocal_forbidden(const SkillRecord& skill,std::span<const Entry> entries,Time now){
    if(skill.type!=15&&skill.type!=27)return false;
    for(const auto& entry:entries)if(entry.skill==883&&entry.kind==Category::hex&&
        entry.cause&&entry.cause<=65535&&control_hex(883,entry.rank)&&
        entry.applied<=now&&entry.expires&&entry.applied<*entry.expires&&now<*entry.expires)return true;
    return false;
}
inline bool control_skill_forbidden(const SkillRecord& skill,std::span<const Entry> entries,Time now){
    return signet_forbidden(skill,entries,now)||vocal_forbidden(skill,entries,now);
}
inline bool adrenaline_forbidden(std::span<const Entry> entries,Time now){
    return active_control_hex(entries,56,now);
}
}
