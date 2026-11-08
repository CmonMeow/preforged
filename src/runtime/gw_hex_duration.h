#pragma once
#include "gw_effect_registry.h"
#include <algorithm>
#include <span>

namespace fw::gw::skills::effects {
class HexDuration;
inline std::optional<HexDuration> hex_duration(unsigned,Time,Scope,std::span<const Entry>,Time);
// A reviewed duration adjustment, separate from the immutable damage/control
// plan. Only the caster-owned stance snapshot can construct one.
class HexDuration {
    unsigned skill_;Time base_,duration_;Scope caster_;
    HexDuration(unsigned skill,Time base,Time duration,Scope caster):
        skill_(skill),base_(base),duration_(duration),caster_(caster){}
    friend std::optional<HexDuration> hex_duration(unsigned,Time,Scope,std::span<const Entry>,Time);
public:
    std::optional<Time> for_hex(unsigned skill,Time base,unsigned cause)const{
        if(skill!=skill_||base!=base_||cause!=caster_.actor)return {};
        return duration_;
    }
};
inline std::optional<HexDuration> hex_duration(unsigned skill,Time base,Scope caster,
        std::span<const Entry> active,Time now){
    const auto* row=find_skill(skill);
    if(!row||row->type!=4||!base||!caster.instance||!caster.actor||!caster.generation)return {};
    unsigned percent=0;
    if(row->attribute==1)for(const auto& e:active){
        if(e.skill!=14||e.kind!=Category::stance||!e.instance||e.cause!=caster.actor||e.rank>20||
           e.applied>now||!e.expires||now>=*e.expires)continue;
        percent=std::max(percent,10+2*e.rank);
    }
    // Preserve millisecond scheduling; do not round each bonus to seconds.
    // Retail sub-second rounding/other duration modifiers remain verification gaps.
    if(base>(UINT64_MAX-50)/150)return {};
    const auto duration=(base*(100+percent)+50)/100;
    if(now>UINT64_MAX-duration)return {};
    return HexDuration{skill,base,duration,caster};
}
}
