#pragma once
#include "gw_overcast_curve.h"
#include "gw_energy_regeneration.h"
namespace fw::gw::overcast {
struct Settlement {
    CeilingCurve curve;vitals::Resources resources;energy::Regeneration regeneration;
    std::uint32_t debt=0;unsigned fractional_tail=0;
    std::uint64_t gained_units=0; // 300000 units per Energy.
    std::uint64_t lost_units=0;
    std::optional<std::uint64_t> first_zero;
    // Precise Energy one millisecond before first_zero. This can be positive
    // even when the complete requested interval began at zero.
    std::optional<std::uint64_t> first_zero_before_units;
};
inline constexpr std::int64_t energy_unit=300000;
inline std::int64_t precise_current(const Settlement& state){
    return (std::int64_t(state.resources.energy)-state.debt)*energy_unit+
        std::int64_t(state.regeneration.remainder())*100+state.fractional_tail;
}
inline bool project_current(Settlement& state,std::int64_t next){
    if(next< -1000000*energy_unit||next>std::int64_t(state.resources.max_energy)*energy_unit)return false;
    const auto whole=next>=0?next/energy_unit:-((-next+energy_unit-1)/energy_unit);
    const auto fraction=static_cast<std::uint64_t>(next-whole*energy_unit);
    state.resources.energy=whole>0?static_cast<unsigned>(whole):0;
    state.debt=whole<0?static_cast<unsigned>(-whole):0;
    state.regeneration.restore_fraction(fraction/100);state.fractional_tail=static_cast<unsigned>(fraction%100);
    return true;
}
// One constant-pip resource-event span. Timed effects must be settled by the
// caller at their actual boundaries; long constant spans do not discard time.
// Constant pips plus a monotonically recovering ceiling can be integrated
// analytically. A shared1/300000-Energy grid represents both pip-ms (100 units)
// and the ceiling clock (3 units), avoiding integer-boundary delays/drift.
inline std::optional<Settlement> settle_constant(const CeilingCurve& original,skills::effects::Scope owner,
        const vitals::Resources& resources,const energy::Regeneration& regeneration,
        std::uint32_t debt,std::uint64_t at,int pips,unsigned fractional_tail=0){
    if(at<original.cursor()||pips< -10||pips>10||
       !resources.max_energy||resources.max_energy>1000000||resources.energy>resources.max_energy||
       debt>1000000||(debt&&resources.energy)||fractional_tail>=100)return {};
    Settlement result{original,resources,regeneration,debt,fractional_tail};
    if(!result.curve.advance(owner,original.cursor()))return {};
    constexpr auto unit=energy_unit;
    const auto limit=result.curve.integer_ceiling(resources.max_energy);
    if(limit< -1000000)return {};
    const auto current=(std::int64_t(resources.energy)-debt)*unit+std::int64_t(regeneration.remainder())*100+fractional_tail;
    const auto ceiling=std::int64_t(resources.max_energy)*unit-std::int64_t(original.burden_units())*3;
    if(current>ceiling)return {};
    if(!resources.health){if(!result.curve.advance(owner,at))return {};return result;}
    if(!result.curve.advance(owner,at))return {};
    if(pips<0){
        // Zero-terminating hex proposal: retain the first crossing for its
        // owner to process BEFORE committing this full-span resource proposal.
        // Do not turn pre-existing capacity-change debt into more degeneration.
        if(current<=0)return result;
        const auto rate=std::uint64_t(-pips)*100;
        const auto until_zero=std::uint64_t(current)/rate+(std::uint64_t(current)%rate!=0);
        const auto elapsed=at-original.cursor();
        const auto lost=elapsed>=until_zero?std::uint64_t(current):elapsed*rate;
        if(!project_current(result,current-static_cast<std::int64_t>(lost)))return {};
        result.lost_units=lost;
        if(elapsed>=until_zero){
            result.first_zero=original.cursor()+until_zero;
            result.first_zero_before_units=std::uint64_t(current)-(until_zero-1)*rate;
        }
        return result;
    }
    const auto final_ceiling=std::int64_t(resources.max_energy)*unit-std::int64_t(result.curve.burden_units())*3;
    const auto available=static_cast<std::uint64_t>(final_ceiling-current);
    const auto rate=static_cast<std::uint64_t>(pips)*100;
    const auto elapsed=at-original.cursor();std::uint64_t gained=0;
    if(rate){
        const auto until_full=available/rate+(available%rate!=0);
        gained=elapsed>=until_full?available:elapsed*rate;
    }
    const auto next=current+static_cast<std::int64_t>(gained);
    if(!project_current(result,next))return {};
    result.gained_units=static_cast<std::uint64_t>(next-current);
    return result;
}
// Every resource proposal below uses this entry point. Split at owned modifier
// boundaries before capping pips; retain the exact remainder and moving ceiling
// across segments. A failed proposal never mutates its input authority.
inline std::optional<Settlement> settle(const CeilingCurve& original,skills::effects::Scope owner,
        const vitals::Resources& resources,const energy::Regeneration& regeneration,
        std::uint32_t debt,std::uint64_t at,int base_pips,unsigned fractional_tail=0){
    if(at<original.cursor()||base_pips< -10||base_pips>10)return {};
    const auto first=original.next_energy_boundary(original.cursor());
    if(!first||*first>=at)return settle_constant(original,owner,resources,regeneration,debt,at,
        original.effective_energy_pips(base_pips,original.cursor()),fractional_tail);
    auto state=settle_constant(original,owner,resources,regeneration,debt,original.cursor(),
        original.effective_energy_pips(base_pips,original.cursor()),fractional_tail);
    if(!state)return {};
    std::uint64_t total=0,lost=0;std::optional<std::uint64_t> zero,zero_before;
    while(state->curve.cursor()<at){
        const auto from=state->curve.cursor();const auto edge=state->curve.next_energy_boundary(from);
        const auto until=edge?std::min(at,*edge):at;
        auto next=settle_constant(state->curve,owner,state->resources,state->regeneration,state->debt,until,
            state->curve.effective_energy_pips(base_pips,from),state->fractional_tail);
        if(!next)return {};total+=next->gained_units;lost+=next->lost_units;
        if(next->first_zero&&(!zero||*next->first_zero<*zero)){
            zero=next->first_zero;zero_before=next->first_zero_before_units;
        }
        state=std::move(next);
    }
    state->gained_units=total;state->lost_units=lost;state->first_zero=zero;
    state->first_zero_before_units=zero_before;return state;
}
// Resource owners must observe zero before later expiry/gain can hide it.
// Return an authority snapshot at that boundary; after committing reactions,
// the owner settles the remaining interval using the changed effect curve.
inline std::optional<Settlement> settle_until_zero(const CeilingCurve& original,skills::effects::Scope owner,
        const vitals::Resources& resources,const energy::Regeneration& regeneration,
        std::uint32_t debt,std::uint64_t at,int base_pips,unsigned fractional_tail=0){
    auto proposed=settle(original,owner,resources,regeneration,debt,at,base_pips,fractional_tail);
    if(!proposed||!proposed->first_zero||*proposed->first_zero==at)return proposed;
    const auto boundary=*proposed->first_zero;
    auto stopped=settle(original,owner,resources,regeneration,debt,boundary,base_pips,fractional_tail);
    if(!stopped||stopped->first_zero!=boundary||precise_current(*stopped)!=0)return {};
    return stopped;
}
struct GainProposal {
    Settlement state;
    std::uint64_t gained_units=0,debt_repaid_units=0,positive_gain_units=0;
};
// Reward authorization/replay protection belongs to the calling skill's owned
// event transaction. This proposal only settles and projects the actual gain.
inline std::optional<GainProposal> prepare_gain_units(const CeilingCurve& original,
        skills::effects::Scope owner,const vitals::Resources& resources,
        const energy::Regeneration& regeneration,std::uint32_t debt,std::uint64_t amount_units,
        std::uint64_t at,int pips,unsigned fractional_tail=0,std::optional<unsigned> reward_ceiling={}){
    if(!resources.health||(reward_ceiling&&*reward_ceiling>1000000))return {};
    auto state=settle(original,owner,resources,regeneration,debt,at,pips,fractional_tail);
    // A resource mutation cannot leap over an unhandled regeneration-driven
    // zero event, even when later recovery hid it or it occurs exactly at at.
    // Settle/dispatch that boundary first, then retry from the committed state.
    if(!state||state->first_zero)return {};
    const auto current=precise_current(*state);
    const auto native_ceiling=std::int64_t(resources.max_energy)*energy_unit-std::int64_t(state->curve.burden_units())*3;
    if(current>native_ceiling)return {};
    // A skill reward ceiling constrains this gain only,not regeneration or
    // capacity. Never lower Energy already above that skill's ceiling.
    const auto ceiling=reward_ceiling?std::min(native_ceiling,std::int64_t(*reward_ceiling)*energy_unit):native_ceiling;
    // Cap in unsigned space before narrowing. Even an oversized authorized
    // reward must not wrap into a negative gain or erase a fractional tail.
    const auto accepted=static_cast<std::int64_t>(std::min(amount_units,
        static_cast<std::uint64_t>(std::max<std::int64_t>(0,ceiling-current))));
    if(!project_current(*state,current+accepted))return {};
    const auto repaid=std::min(accepted,std::max<std::int64_t>(0,-current));
    return GainProposal{std::move(*state),static_cast<std::uint64_t>(accepted),static_cast<std::uint64_t>(repaid),static_cast<std::uint64_t>(accepted-repaid)};
}
inline std::optional<GainProposal> prepare_gain(const CeilingCurve& original,
        skills::effects::Scope owner,const vitals::Resources& resources,
        const energy::Regeneration& regeneration,std::uint32_t debt,unsigned amount,
        std::uint64_t at,int pips,unsigned fractional_tail=0,std::optional<unsigned> reward_ceiling={}){
    return prepare_gain_units(original,owner,resources,regeneration,debt,
        std::uint64_t(amount)*energy_unit,at,pips,fractional_tail,reward_ceiling);
}
enum class LossRule {full_cost,drain_to_zero};
struct LossProposal {Settlement state;std::uint64_t lost_units=0;};
inline std::optional<LossProposal> prepare_loss(const CeilingCurve& original,
        skills::effects::Scope owner,const vitals::Resources& resources,
        const energy::Regeneration& regeneration,std::uint32_t debt,unsigned amount,
        std::uint64_t at,int pips,LossRule rule,unsigned fractional_tail=0){
    if(!resources.health||(rule!=LossRule::full_cost&&rule!=LossRule::drain_to_zero))return {};
    auto state=settle(original,owner,resources,regeneration,debt,at,pips,fractional_tail);
    if(!state||state->first_zero)return {};
    const auto current=precise_current(*state),requested=std::int64_t(amount)*energy_unit;
    if(rule==LossRule::full_cost&&amount&&current<requested)return {};
    const auto lost=rule==LossRule::full_cost?requested:std::min(requested,std::max<std::int64_t>(0,current));
    if(!project_current(*state,current-lost))return {};
    return LossProposal{std::move(*state),static_cast<std::uint64_t>(lost)};
}
struct ResizeProposal {Settlement state;std::int64_t current_delta_units=0;};
// Ordinary nominal-capacity changes move current by the same delta. This is
// distinct from Overcast application/recovery, which must not grant Energy.
inline std::optional<ResizeProposal> prepare_resize(const CeilingCurve& original,
        skills::effects::Scope owner,const vitals::Resources& resources,
        const energy::Regeneration& regeneration,std::uint32_t debt,unsigned nominal,
        std::uint64_t at,int pips,unsigned fractional_tail=0){
    if(!nominal||nominal>1000000)return {};
    auto state=settle(original,owner,resources,regeneration,debt,at,pips,fractional_tail);
    if(!state||state->first_zero)return {};
    const auto delta=(std::int64_t(nominal)-resources.max_energy)*energy_unit;
    const auto next=precise_current(*state)+delta;state->resources.max_energy=nominal;
    if(state->curve.integer_ceiling(nominal)< -1000000||!project_current(*state,next))return {};
    return ResizeProposal{std::move(*state),delta};
}
// The resurrection owner supplies its reviewed integer Energy entitlement.
// Recover the dead actor's ceiling first; a resurrection cannot erase burden
// or grant Energy above that ceiling. Retail percentage-base/rounding evidence
// remains the resurrection policy's responsibility.
inline std::optional<Settlement> prepare_resurrection(const CeilingCurve& original,
        skills::effects::Scope owner,const vitals::Resources& resources,
        const energy::Regeneration& regeneration,std::uint32_t debt,unsigned granted,
        std::uint64_t at,unsigned fractional_tail=0){
    if(resources.health||granted>resources.max_energy)return {};
    auto state=settle(original,owner,resources,regeneration,debt,at,0,fractional_tail);
    if(!state)return {};
    const auto ceiling=std::int64_t(resources.max_energy)*energy_unit-std::int64_t(state->curve.burden_units())*3;
    if(!project_current(*state,std::min(std::int64_t(granted)*energy_unit,ceiling)))return {};
    return state;
}

}
