#pragma once
#include "gw_berserker_stance.h"
#include "gw_bestial_fury.h"
#include "gw_dwarven_battle_stance.h"
#include "gw_skill_effect_plan.h"
#include "gw_health_regeneration.h"
#include "gw_frenzy.h"
#include "gw_tiger_stance.h"
#include "gw_hekets_rampage.h"
#include "gw_flurry.h"
#include "gw_flail.h"
#include "gw_lightning_reflexes.h"
namespace fw::gw::skills::effects {
// Local Faintheartedness revision2648713, Attack_speed and Effect_stacking;
// pinned2022 row135. Scale is attack-duration increase; BONUS is degeneration.
struct FaintheartednessPlan {
    unsigned rank=0,pips=0,slower_percent=50;Time duration=0;float range=1248;
    unsigned skill=135;float radius=0;
    auto operator<=>(const FaintheartednessPlan&)const=default;
};
inline std::optional<FaintheartednessPlan> faintheartedness(unsigned rank){
    const auto* row=find_skill(135);
    if(rank>20||!row||row->type!=4||row->attribute!=7)return {};
    return FaintheartednessPlan{rank,interpolate(row->bonus0,row->bonus15,rank),
        interpolate(row->scale0,row->scale15,rank),Time(interpolate(3,16,rank))*1000};
}
inline std::optional<FaintheartednessPlan> shadow_of_fear(unsigned rank){
    if(rank>20)return {};
    return FaintheartednessPlan{rank,0,50,Time(interpolate(5,30,rank))*1000,1248,136,166};
}
inline std::optional<FaintheartednessPlan> attack_slow_hex(unsigned skill,unsigned rank){
    if(skill==135)return faintheartedness(rank);
    if(skill==136)return shadow_of_fear(rank);
    if(skill!=1260||rank>20)return {};
    const auto* row=find_skill(skill);
    if(!row||row->type!=4||row->profession!=4||row->attribute!=7||row->special||row->energy_cost()!=15||
       row->health_sacrifice!=17||row->activation!=2.f||row->aftercast!=.75f||row->recharge!=15||
       row->duration0!=5||row->duration15!=30||row->scale0!=17||row->scale15!=17||row->bonus0!=50||row->bonus15!=50||
       row->caster_body_resource!=246||row->target_body_resource!=2077)return {};
    // Archived Meekness: in the area.322 is the archived range convention,
    // not a distance recovered from the activation capture.
    return FaintheartednessPlan{rank,0,50,Time(interpolate(5,30,rank))*1000,1248,1260,322};
}
inline bool valid_faintheartedness(const FaintheartednessPlan& p){const auto e=attack_slow_hex(p.skill,p.rank);return e&&*e==p;}
inline bool faintheartedness_active(std::span<const Entry> entries,Time now){
    for(const auto& entry:entries)if((entry.skill==135||entry.skill==136||entry.skill==1260||entry.skill==54)&&entry.kind==Category::hex&&entry.rank<=20&&
        entry.applied<=now&&entry.expires&&now<*entry.expires)return true;
    return false;
}
// Only reviewed attack-speed effects are admitted, including PvE Crippling Anguish.
// Its PvP alias3152 intentionally supplies no attack slow. Multiplicative
// duration modifiers: Frenzy 0.67, Faintheartedness 1.50, combined 1.005.
// Round once at the scheduling boundary, not once per effect. This applies to
// weapon windup AND repeat period, including attack skills; never spell casts.
inline std::optional<Time> reviewed_attack_duration(Time base,bool frenzy,bool fainthearted,bool bestial=false){
    if(base>60000)return {};
    const unsigned multiplier=(frenzy?67:bestial?80:100)*(fainthearted?150:100);
    return (base*multiplier+5000)/10000;
}
inline std::optional<combat::weapon::Timing> reviewed_attack_timing(combat::weapon::Timing base,
        std::span<const Entry> entries,Time now,bool skill_use=false,std::optional<bool> hammer={}){
    if(!base.period_ms||base.period_ms>60000||base.windup_ms>base.period_ms||
       !std::isfinite(base.range)||base.range<=0||base.range>5000)return {};
    const auto dwarven=dwarven_battle_bonuses(entries,hammer,now);if(!dwarven)return {};
    const bool fast=dwarven->haste||tiger_stance_active(entries,now)||hekets_rampage_active(entries,now)||(!skill_use&&berserker_stance_active(entries,now))||frenzy_active(entries,now)||flurry_active(entries,now)||lightning_reflexes_active(entries,now)||flail_active(entries,now),slow=faintheartedness_active(entries,now);
    const bool bestial=bestial_fury_active(entries,now);
    base.windup_ms=*reviewed_attack_duration(base.windup_ms,fast,slow,bestial);
    base.period_ms=std::max<Time>(1,*reviewed_attack_duration(base.period_ms,fast,slow,bestial));return base;
}
struct FaintheartednessApplication {Registry registry;regeneration::State regeneration;Entry entry;Applied effects;};
// World advances authoritative health to now first. Plan both status and pips
// atomically; commit returned copies together, then publish add/remove packets.
inline std::optional<FaintheartednessApplication> apply_faintheartedness(Scope target,
        const Registry& registry,const regeneration::State& regeneration,
        std::uint32_t effect,std::uint32_t caster,const FaintheartednessPlan& plan,Time now){
    if(!valid_faintheartedness(plan)||!caster||now>UINT64_MAX-plan.duration)return {};
    const Entry entry{effect,caster,plan.skill,plan.rank,Category::hex,now,now+plan.duration};
    FaintheartednessApplication result{registry,regeneration,entry,{}};
    result.effects=result.registry.apply(target,entry,Replace::same_skill);
    if(!result.effects.accepted||!result.regeneration.apply(target,{effect,caster,plan.skill,-static_cast<int>(plan.pips),now,now+plan.duration}))return {};
    return result;
}
}
