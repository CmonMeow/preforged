#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_aura_restoration.h"
namespace fw::gw::skills::effects {
struct VigorousSpiritPlan {
    unsigned rank=0,healing=5,energy=5;
    Time duration=30000,activation=250,aftercast=750,recharge=4000;
    auto operator<=>(const VigorousSpiritPlan&)const=default;
};
inline std::optional<VigorousSpiritPlan> vigorous_spirit(unsigned rank){
    const auto* row=find_skill(254);
    if(rank>20||!row||row->profession!=3||row->attribute!=13||row->type!=6||row->special||
       row->energy_cost()!=5||row->health_sacrifice||row->adrenaline||row->activation!=.25f||row->aftercast!=.75f||row->recharge!=4||
       row->duration0!=30||row->duration15!=30||row->scale0!=5||row->scale15!=20||row->bonus0||row->bonus15||
       row->caster_body_resource!=2077||row->target_body_resource!=465)return {};
    return VigorousSpiritPlan{rank,interpolate(5,20,rank)};
}
enum class VigorousActionKind {attack,completed_spell,other_skill};
// Internal authoritative events only: one event per actual attack, not per hit
// or damage recipient. A double attack supplies two separate events. This query
// does not consume/replay-proof tokens; that belongs to the committing World.
struct VigorousAction {
    Scope actor;std::uint64_t event=0;Time at=0;
    VigorousActionKind kind=VigorousActionKind::attack;unsigned skill=0;
    auto operator<=>(const VigorousAction&)const=default;
};
struct VigorousHealing {unsigned effect=0,amount=0;auto operator<=>(const VigorousHealing&)const=default;};
inline std::optional<VigorousHealing> vigorous_spirit_healing(const Registry& registry,const VigorousAction& action){
    const auto owner=action.actor;
    if(!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!action.event||!registry.matches(owner))return {};
    if(action.kind!=VigorousActionKind::attack&&action.kind!=VigorousActionKind::completed_spell&&action.kind!=VigorousActionKind::other_skill)return {};
    if(action.kind!=VigorousActionKind::attack){
        const auto* row=find_skill(action.skill);if(!row||!row->id||!row->type)return {};
        if(action.kind==VigorousActionKind::other_skill)return VigorousHealing{};
        if(!is_spell(*row))return {};
    }
    const auto entry=registry.newest_skill(owner,254,action.at);if(!entry)return VigorousHealing{};
    const auto plan=vigorous_spirit(entry->rank);
    // An ally's enchantment is valid: cause need not equal the recipient.
    // Equipment duration is validated by its application producer, not guessed
    // from a fixed30second end here.
    if(!plan||!entry->instance||!entry->cause||entry->cause>65535||entry->kind!=Category::enchantment||
       !entry->expires||*entry->expires<=entry->applied)return {};
    return VigorousHealing{entry->instance,plan->healing};
}
}
