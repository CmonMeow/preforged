#pragma once
#include "gw_effect_registry.h"
#include "gw_weapon_intent.h"
#include <span>
namespace fw::gw::skills::effects {
// Local Frenzy/Attack_speed wiki: 8 seconds, 33% duration reduction (not
// division by1.33), double strike damage. No rank scaling. Not degeneration,
// sacrifice or life stealing: those must not use the strike-damage multiplier.
struct FrenzyPlan {unsigned skill=346;Time duration=8000;unsigned energy=5,recharge=4000;};
inline std::optional<FrenzyPlan> frenzy(unsigned skill){
    if(skill!=346)return {};
    const auto* row=find_skill(skill);
    if(!row||row->type!=3||row->energy_cost()!=5||row->recharge!=4)return {};
    return FrenzyPlan{};
}
inline bool frenzy_active(std::span<const Entry> entries,Time now){
    for(const auto& entry:entries)
        if(entry.skill==346&&entry.kind==Category::stance&&entry.applied<=now&&
           entry.expires&&now<*entry.expires)return true;
    return false;
}
inline std::optional<combat::weapon::Timing> frenzy_timing(combat::weapon::Timing base,bool active){
    if(!base.period_ms||base.period_ms>60000||base.windup_ms>base.period_ms||
       !std::isfinite(base.range)||base.range<=0||base.range>5000)return {};
    if(!active)return base;
    // Millisecond rounding only at the scheduler boundary. Preserve range.
    base.windup_ms=(base.windup_ms*67+50)/100;
    base.period_ms=std::max<std::uint64_t>(1,(base.period_ms*67+50)/100);return base;
}
inline std::optional<double> frenzy_strike_damage(double damage,bool active){
    if(!std::isfinite(damage)||damage<0||damage>1000000)return {};
    const double result=damage*(active?2:1);
    if(result>1000000)return {};return result;
}
}
