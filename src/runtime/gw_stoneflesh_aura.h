#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_weapon_attack.h"
#include <optional>

namespace fw::gw::skills::effects {
struct StonefleshAuraPlan {
    unsigned rank=0,reduction=1;
    Time duration=5000;
    bool operator==(const StonefleshAuraPlan&)const=default;
};
inline std::optional<StonefleshAuraPlan> stoneflesh_aura(unsigned rank){
    const auto* row=find_skill(1375);
    if(rank>20||!row||row->campaign!=3||row->profession!=6||row->attribute!=9||
       row->type!=6||row->target||row->energy_cost()!=10||row->health_sacrifice||
       row->adrenaline||row->activation!=2.f||row->aftercast!=.75f||
       row->recharge!=15||row->scale0!=1||row->scale15!=31||
       row->duration0!=5||row->duration15!=15||row->caster_body_resource!=1184||
       row->target_body_resource!=2077)return {};
    return StonefleshAuraPlan{rank,interpolate(1,31,rank),
        1000ull*interpolate(5,15,rank)};
}
struct StonefleshAuraApplication {
    Registry registry;
    Entry applied;
    std::vector<Entry> replaced;
};
inline std::optional<StonefleshAuraApplication> prepare_stoneflesh_aura(
        const Registry& current,Scope owner,unsigned effect,
        const StonefleshAuraPlan& plan,Time now){
    if(stoneflesh_aura(plan.rank)!=plan||!owner.instance||!owner.actor||
       !owner.generation||!current.matches(owner)||!effect||
       now>UINT64_MAX-plan.duration)return {};
    Registry proposed=current;
    const Entry applied{effect,owner.actor,1375,plan.rank,
        Category::enchantment,now,now+plan.duration};
    const auto result=proposed.apply(owner,applied,Replace::same_skill);
    if(!result.accepted)return {};
    return StonefleshAuraApplication{std::move(proposed),applied,result.removed};
}
inline std::optional<StonefleshAuraPlan> active_stoneflesh_aura(
        const Registry& registry,Scope owner,Time now){
    const auto entry=registry.newest_skill(owner,1375,now);
    if(!entry||entry->kind!=Category::enchantment||entry->cause!=owner.actor||
       !entry->instance||!entry->expires)return {};
    const auto plan=stoneflesh_aura(entry->rank);
    if(!plan||entry->applied>UINT64_MAX-plan->duration||
       *entry->expires<entry->applied+plan->duration)return {};
    // Equipment quotes are validated by the inserting authority. Accept only
    // the reviewed upper bound here, never an arbitrary indefinite record.
    const auto seconds=plan->duration/1000;
    const auto upper=(seconds+std::max(Time(1),(seconds*20+50)/100))*1000;
    if(entry->applied>UINT64_MAX-upper||*entry->expires>entry->applied+upper)return {};
    return plan;
}
// A removal capability is captured before mutation and is valid only for that
// actor lifetime and instant. It cannot revive an expired or unrelated effect.
class EndingStonefleshContext {
    Scope target_;Time at_;StonefleshAuraPlan plan_;
    EndingStonefleshContext(Scope target,Time at,StonefleshAuraPlan plan):target_(target),at_(at),plan_(plan){}
public:
    static std::optional<EndingStonefleshContext> capture(const Registry& registry,
            Scope target,unsigned effect,Time now){
        const auto entry=registry.newest_skill(target,1375,now);
        const auto plan=active_stoneflesh_aura(registry,target,now);
        if(!entry||entry->instance!=effect||!plan)return {};
        return EndingStonefleshContext(target,now,*plan);
    }
    std::optional<StonefleshAuraPlan> plan(Scope target,Time now)const{
        return target==target_&&now==at_?std::optional(plan_):std::nullopt;
    }
};
// World must validate equipment duration and retain the ending enchantment
// context for a damaging strip. These adapters consume an already validated
// plan; they do not infer that an actor currently owns the enchantment.
inline bool apply_stoneflesh_damage(combat::Actor& actor,const StonefleshAuraPlan& plan){
    if(stoneflesh_aura(plan.rank)!=plan||!std::isfinite(actor.incoming_reduction)||
       actor.incoming_reduction<0||actor.incoming_reduction>1000000-plan.reduction)return false;
    // Preserve a separate life-steal authority: this enchantment reduces damage,
    // not life stealing, degeneration or health sacrifice.
    if(!actor.life_steal_reduction)actor.life_steal_reduction=actor.incoming_reduction;
    actor.incoming_reduction+=plan.reduction;
    return true;
}
inline bool apply_stoneflesh_critical(combat::weapon::Strike& strike,
        const StonefleshAuraPlan& plan){
    if(stoneflesh_aura(plan.rank)!=plan)return false;
    strike.critical_damage_suppressed=true;
    strike.critical_energy_suppressed=true;
    return true;
}
}
