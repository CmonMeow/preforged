#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_aura_restoration.h"
#include "gw_effect_registry.h"
#include "gw_skillbar.h"
#include "gw_cooldown_bar_authority.h"
#include <compare>

namespace fw::gw::skills::effects {
struct ElementalGlyphPlan {
    unsigned skill=0,rank=0,charges=0,elemental_bonus=0,energy_reduction=0;
    std::optional<std::uint64_t> duration;
    bool suppress_base_overcast=false,instant_recharge=false;
    auto operator<=>(const ElementalGlyphPlan&)const=default;
};
inline std::optional<ElementalGlyphPlan> elemental_glyph(unsigned skill,unsigned storage_rank=0){
    if(storage_rank>20||(skill!=198&&skill!=199&&skill!=203))return {};
    const auto* row=find_skill(skill);
    if(!row||row->type!=12||row->encoded_energy!=5||row->activation!=1.f||row->aftercast!=.75f||row->pvp_skill!=3433||
       row->health_sacrifice||row->adrenaline||(row->special&1))return {};
    if(skill==198){
        if(row->attribute!=51||row->recharge!=5||row->bonus0!=10||row->bonus15!=10||row->duration0!=25||row->duration15!=25)return {};
        return ElementalGlyphPlan{198,0,10,2,0,25000,false,false};
    }
    if(skill==199){
        if(row->attribute!=12||!(row->special&4)||row->recharge!=25||row->scale0!=10||row->scale15!=25||
           row->bonus0!=1||row->bonus15!=2||row->duration0!=1||row->duration15!=3)return {};
        // This row's duration endpoints are CHARGES. GoE has no expiration.
        return ElementalGlyphPlan{199,storage_rank,interpolate(1,3,storage_rank),interpolate(1,2,storage_rank),
            interpolate(10,25,storage_rank),{},true,false};
    }
    if(row->attribute!=51||!(row->special&4)||row->recharge!=10||row->duration0!=15||row->duration15!=15)return {};
    return ElementalGlyphPlan{203,0,1,0,0,15000,false,true};
}
struct ElementalGlyphState {
    Scope owner;std::uint32_t effect=0;ElementalGlyphPlan plan;
    unsigned remaining=0;std::uint64_t began=0;std::optional<std::uint64_t> expires;
    bool operator==(const ElementalGlyphState&)const=default;
};
inline bool valid_elemental_glyph(const ElementalGlyphState& state){
    const auto canonical=elemental_glyph(state.plan.skill,state.plan.rank);
    if(!canonical||*canonical!=state.plan||!state.owner.instance||!state.owner.actor||state.owner.actor>65535||
       !state.owner.generation||!state.effect||!state.remaining||state.remaining>state.plan.charges)return false;
    if(state.plan.duration)return state.began<=UINT64_MAX-*state.plan.duration&&state.expires==state.began+*state.plan.duration;
    return !state.expires;
}
inline bool registered_elemental_glyph(const Registry& registry,const ElementalGlyphState& state){
    if(!valid_elemental_glyph(state))return false;
    const auto entry=registry.lookup(state.owner,state.effect);
    return entry&&entry->cause==state.owner.actor&&entry->skill==state.plan.skill&&entry->rank==state.plan.rank&&
        entry->kind==Category::glyph&&entry->applied==state.began&&entry->expires==state.expires;
}
inline std::optional<ElementalGlyphState> prepare_elemental_glyph(Scope owner,std::uint32_t effect,
    const ElementalGlyphPlan& plan,std::uint64_t now){
    const auto canonical=elemental_glyph(plan.skill,plan.rank);
    if(!canonical||*canonical!=plan||(plan.duration&&now>UINT64_MAX-*plan.duration))return {};
    ElementalGlyphState state{owner,effect,plan,plan.charges,now,plan.duration?std::optional(now+*plan.duration):std::nullopt};
    return valid_elemental_glyph(state)?std::optional(state):std::nullopt;
}
inline bool elemental_glyph_live(const ElementalGlyphState& state,std::uint64_t now){
    return valid_elemental_glyph(state)&&now>=state.began&&(!state.expires||now<*state.expires);
}
// All glyphs replace the previous glyph after successful activation. They are
// not enchantments, and the incoming glyph itself is not a spell trigger.
inline bool replaces_elemental_glyph(unsigned incoming_skill){
    const auto* row=find_skill(incoming_skill);return row&&row->id&&row->type==12;
}
inline std::optional<unsigned> elemental_glyph_attribute_bonus(const ElementalGlyphState& state,unsigned attribute,std::uint64_t now){
    if(!valid_elemental_glyph(state)||attribute>51)return {};
    return elemental_glyph_live(state,now)&&attribute>=8&&attribute<=11?state.plan.elemental_bonus:0;
}
struct ElementalGlyphQuote {
    ElementalGlyphState glyph;Slot slot;
    unsigned energy_cost=0,base_overcast=0,elemental_bonus=0;
    std::uint64_t at=0;
};
// Supply the caller's already-reviewed cost at the correct modifier stage.
// Only BASE spell Overcast is suppressed; external/passive sources remain owned
// by their separate authority. The quote retains the last charge's rank bonus.
inline std::optional<ElementalGlyphQuote> quote_elemental_glyph(const ElementalGlyphState& state,Slot slot,
    unsigned energy_cost,unsigned base_overcast,std::uint64_t now){
    const auto* row=find_skill(slot.skill);
    if(!elemental_glyph_live(state,now)||!row||!row->id||!is_spell(*row)||slot.copy>65535||energy_cost>1000000||base_overcast>1000000)return {};
    return ElementalGlyphQuote{state,slot,energy_cost>state.plan.energy_reduction?energy_cost-state.plan.energy_reduction:0,
        state.plan.suppress_base_overcast?0:base_overcast,state.plan.elemental_bonus,now};
}
enum class GlyphPhase {paid_admission,terminal};
enum class GlyphOutcome {admitted,completed,voluntary_cancel,interrupted,failed,not_admitted};
struct ElementalGlyphTrigger {bool supported=false,consume=false,instant_recharge=false,reserve=false;};
inline ElementalGlyphTrigger elemental_glyph_trigger(const ElementalGlyphPlan& plan,GlyphPhase phase,GlyphOutcome outcome=GlyphOutcome::admitted){
    const auto canonical=elemental_glyph(plan.skill,plan.rank);if(!canonical||*canonical!=plan)return {};
    if(phase!=GlyphPhase::paid_admission&&phase!=GlyphPhase::terminal)return {};
    if(outcome!=GlyphOutcome::admitted&&outcome!=GlyphOutcome::completed&&outcome!=GlyphOutcome::voluntary_cancel&&outcome!=GlyphOutcome::interrupted&&
       outcome!=GlyphOutcome::failed&&outcome!=GlyphOutcome::not_admitted)return {};
    if(outcome==GlyphOutcome::not_admitted)return {true,false,false};
    if(phase==GlyphPhase::paid_admission)return outcome==GlyphOutcome::admitted?ElementalGlyphTrigger{true,false,false,true}:ElementalGlyphTrigger{};
    if(outcome==GlyphOutcome::admitted||outcome==GlyphOutcome::failed)return {};
    // Energy irrevocably reserves its use at admitted payment, including a
    // later cancellation/interruption. True failure remains unmeasured.
    // The captured final effect/rank
    // removal occurs at the following spell's terminal event, not its start.
    if(plan.skill==199)return {true,true,false};
    if(outcome==GlyphOutcome::voluntary_cancel)return {true,false,false};
    if(outcome==GlyphOutcome::completed)return {true,true,plan.instant_recharge};
    if(plan.skill==203&&outcome==GlyphOutcome::interrupted)return {true,true,true};
    // No direct evidence here resolves true failure, or198 interruption.
    return {};
}
struct ElementalGlyphConsumption {bool accepted=false;std::optional<ElementalGlyphState> remaining;};
// Pure, optimistic charge transaction. Call only for a supported consume=true
// trigger; commit under Runtime/World authority at the appropriate terminal
// event after actual admission. Replacement/recycled owners/stale snapshots
// cannot consume another glyph's charge. Prefer the event-bound lease below
// for terminal lifecycle settlement.
inline ElementalGlyphConsumption consume_elemental_glyph(const ElementalGlyphState& current,const ElementalGlyphQuote& quote){
    const auto* row=find_skill(quote.slot.skill);
    if(current!=quote.glyph||!elemental_glyph_live(current,quote.at)||!row||!is_spell(*row)||quote.slot.copy>65535||
       quote.elemental_bonus!=current.plan.elemental_bonus)return {};
    auto next=current;if(!--next.remaining)return {true,{}};
    return {true,std::move(next)};
}
struct ElementalGlyphLease {ElementalGlyphQuote quote;std::uint64_t event=0;};
// Pure reservation: no charge decrement or effect removal. The owner must store
// exactly one pending lease under its authoritative cast transaction lock.
inline std::optional<ElementalGlyphLease> reserve_elemental_glyph(const ElementalGlyphState& current,
    const ElementalGlyphQuote& quote,std::uint64_t event){
    if(!event||!consume_elemental_glyph(current,quote).accepted)return {};
    return ElementalGlyphLease{quote,event};
}
struct ElementalGlyphSettlement {
    bool accepted=false,supported=false;
    std::optional<ElementalGlyphState> remaining;
    bool consumed=false,instant_recharge=false;
};
struct RenewalTerminal {
    CooldownCompletion completion;
    unsigned glyph_effect=0;
    std::uint64_t control_epoch=0;
    std::uint64_t independent_disable_until=0;
};
inline ElementalGlyphSettlement settle_elemental_glyph(const std::optional<ElementalGlyphState>& current,
    const ElementalGlyphLease& lease,std::uint64_t event,GlyphOutcome outcome,std::uint64_t now){
    if(!event||event!=lease.event||!current||now<lease.quote.at||
       !reserve_elemental_glyph(*current,lease.quote,event))return {};
    // No reviewed evidence decides a timed glyph expiring during its spell.
    if(current->expires&&now>=*current->expires)return {};
    const auto trigger=elemental_glyph_trigger(current->plan,GlyphPhase::terminal,outcome);
    if(!trigger.supported)return {};
    if(!trigger.consume)return {true,true,current,false,false};
    const auto proposal=consume_elemental_glyph(*current,lease.quote);
    if(!proposal.accepted)return {};
    return {true,true,proposal.remaining,true,trigger.instant_recharge};
}
inline std::optional<std::uint64_t> elemental_glyph_recharge(const ElementalGlyphPlan& plan,GlyphOutcome outcome,std::uint64_t otherwise){
    const auto trigger=elemental_glyph_trigger(plan,GlyphPhase::terminal,outcome);
    if(!trigger.supported)return {};
    return trigger.instant_recharge?0:otherwise;
}
}
