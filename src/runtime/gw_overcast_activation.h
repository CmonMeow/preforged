#pragma once
#include "gw_overcast_settlement.h"
namespace fw::gw::overcast {
struct Activation {
    Settlement state;unsigned ordinary_paid=0;
    std::uint64_t ceiling_loss_units=0;
};
// Trusted resource transaction only. Runtime/World must validate skill metadata,
// target/admission and modified ordinary cost before committing this proposal.
// Activation pays ordinary Energy first, then clamps only Energy above the new
// ceiling. Cancellation and interruption do not undo a committed activation.
inline std::optional<Activation> prepare_activation(const CeilingCurve& original,
        skills::effects::Scope owner,const vitals::Resources& resources,
        const energy::Regeneration& regeneration,std::uint32_t debt,
        unsigned ordinary_cost,std::uint64_t overcast_cost,std::uint64_t event,
        std::uint64_t at,int pips,unsigned fractional_tail=0){
    if(!resources.health||!overcast_cost||!event)return {};
    auto payment=prepare_loss(original,owner,resources,regeneration,debt,ordinary_cost,at,pips,LossRule::full_cost,fractional_tail);
    if(!payment)return {};
    auto& state=payment->state;
    const auto paid=precise_current(state);
    if(!state.curve.apply(owner,overcast_cost,event,at)||state.curve.integer_ceiling(resources.max_energy)< -1000000)return {};
    const auto ceiling=std::int64_t(resources.max_energy)*energy_unit-std::int64_t(state.curve.burden_units())*3;
    const auto after=std::min(paid,ceiling);
    if(!project_current(state,after))return {};
    return Activation{std::move(state),ordinary_cost,static_cast<std::uint64_t>(paid-after)};
}
}
