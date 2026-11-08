#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include "gw_skillbar.h"
#include <compare>
namespace fw::gw::skills::effects {
struct RustPlan {
    unsigned rank=0;double damage=10;std::uint64_t duration=5000,disable=1000;
    float range=1248,radius=166;
    auto operator<=>(const RustPlan&)const=default;
};
inline std::optional<RustPlan> rust(unsigned rank){
    const auto* row=find_skill(204);
    if(rank>20||!row||row->type!=4||row->attribute!=11||row->scale0!=1||row->scale15!=10||
       row->bonus0!=10||row->bonus15!=70||row->duration0!=5||row->duration15!=20||row->encoded_energy!=10)return {};
    return RustPlan{rank,damage_scale(10,70,rank),1000ull*interpolate(5,20,rank),1000ull*interpolate(1,10,rank)};
}
struct RustActivity {Slot slot;std::uint64_t began=0,ends=0;};
struct RustResult {
    bool accepted=false;combat::Actor actor;combat::Hit hit;std::uint64_t hex_ms=0,disable_ms=0;
    std::optional<Slot> active_signet;
};
// Pure recipient proposal. World must sample current activity and Overcast,
// own hex admission,disable authority and interrupt protection. A disable is
// proposed independently of whether the subsequent interrupt is prevented.
inline RustResult rust_target(const RustPlan& plan,combat::Actor actor,const combat::conditions::State& conditions,
        unsigned level,bool overcast,std::optional<RustActivity> activity,std::uint64_t now){
    const auto canonical=rust(plan.rank);
    if(!canonical||*canonical!=plan||!actor.cold_armor)return {};
    actor.cold_armor=conditions.armor(*actor.cold_armor,now);
    const auto hit=combat::apply(actor,{combat::Kind::cold,plan.damage,level,204},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    RustResult result{true,actor,hit};if(hit.killed)return result;
    if(now>UINT64_MAX-plan.duration)return {};result.hex_ms=plan.duration;
    if(overcast&&activity){
        const auto* active=find_skill(activity->slot.skill);
        if(!active||!activity->slot.skill||activity->slot.copy>65535||activity->ends<=activity->began)return {};
        if(activity->began<=now&&now<activity->ends&&active->type==7){
            if(now>UINT64_MAX-plan.disable)return {};
            result.active_signet=activity->slot;result.disable_ms=plan.disable;
        }
    }
    return result;
}
}
