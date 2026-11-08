#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <cmath>
#include <optional>
#include <vector>
namespace fw::gw::skills::effects {
// Local wiki Fire_Storm: fixed target location, ten one-second pulses,
// adjacent radius, 5..29..35 fire damage at ranks0..12..15.
struct FireStormPlan { unsigned rank=0;double damage=5;float radius=166;unsigned skill=197,pulses=10;
    auto operator<=>(const FireStormPlan&)const=default; };
inline std::optional<FireStormPlan> fire_storm(unsigned rank,unsigned skill=197){
    if(rank>20||(skill!=197&&skill!=1094&&skill!=215&&skill!=167&&skill!=187&&skill!=192))return {};
    if(skill==187||skill==192)return FireStormPlan{rank,damage_scale(7,112,rank),166,skill,skill==187?1u:3u};
    if(skill==167)return FireStormPlan{rank,damage_scale(10,40,rank),252,167,5};
    return FireStormPlan{rank,damage_scale(skill==197?5:10,skill==197?35:skill==215?25:40,rank),166,skill,skill==1094?5u:10u};
}
struct FireStormPulse {Scope caster;unsigned level;float x,y;std::uint16_t plane;
    FireStormPlan plan;std::uint64_t at;};
// A field keeps its original center after the target moves or dies. Callers
// resolve occupants and damage at each pulse time; this is not target tracking.
class FireStormField {
    FireStormPulse pulse_;unsigned emitted_=0;std::uint64_t start_=0,last_=0;
public:
    static std::optional<FireStormField> create(Scope caster,unsigned level,float x,float y,
        std::uint16_t plane,const FireStormPlan& plan,std::uint64_t now){
        const auto expected=fire_storm(plan.rank,plan.skill);
        if(!expected||*expected!=plan||!caster.actor||!caster.generation||!level||level>255||
           !std::isfinite(x)||!std::isfinite(y)||now>UINT64_MAX-std::uint64_t(plan.pulses)*1000)return {};
        FireStormField field;field.pulse_={caster,level,x,y,plane,plan,0};field.start_=field.last_=now;return field;
    }
    std::vector<FireStormPulse> advance(std::uint64_t now){
        std::vector<FireStormPulse> result;if(now<last_)return result;last_=now;
        while(emitted_<pulse_.plan.pulses&&now>=start_+(emitted_+1)*1000){
            auto pulse=pulse_;pulse.at=start_+(++emitted_)*1000;result.push_back(pulse);
        }return result;
    }
    bool finished()const{return emitted_==pulse_.plan.pulses;}
    Scope caster()const{return pulse_.caster;}
};
}
