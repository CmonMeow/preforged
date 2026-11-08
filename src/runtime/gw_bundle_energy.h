#pragma once
#include "gw_overcast_settlement.h"
#include "gw_consumables.h"

namespace fw::gw::bundles {
// Trusted equipment facts, not packet fields. Equipment includes hands; morale
// affects the base through the existing resource policy, not the hand subtraction.
struct EnergySources {
    unsigned base=0;int equipment=0,hands=0;unsigned morale=100;
    std::optional<unsigned> nominal(bool held)const{
        if(!base||base>1000000||equipment< -1000000||equipment>1000000||hands< -1000000||hands>1000000||morale>110)return {};
        try{return consumables::capacity(base,morale,equipment-(held?hands:0));}
        catch(const std::invalid_argument&){return {};}
    }
};
struct EnergyTransition {
    skills::effects::Scope owner;std::uint64_t at=0;
    bool held_before=false,held_after=false;
    overcast::Settlement before,after;
    // Crossing into debt is not necessarily an effect's trigger: Mind Wrack,
    // for example, requires exact real zero. Its own policy must authorize it.
    bool crossed_zero()const{return overcast::precise_current(before)>0&&overcast::precise_current(after)<=0;}
};
// Copy-only preparation. Caller must settle timed effects at their own boundaries,
// commit ownership/resource state, then dispatch the before/after reaction exactly
// once. A zero crossing in the preceding interval belongs to that earlier event.
inline std::optional<EnergyTransition> prepare_energy_transition(const EnergySources& sources,
        bool held_before,bool held_after,const overcast::CeilingCurve& curve,
        skills::effects::Scope owner,const vitals::Resources& resources,
        const energy::Regeneration& regeneration,std::uint32_t debt,
        std::uint64_t at,int pips,unsigned fractional_tail=0){
    const auto old=sources.nominal(held_before),next=sources.nominal(held_after);
    if(held_before==held_after||!old||!next||resources.max_energy!=*old)return {};
    auto before=overcast::settle(curve,owner,resources,regeneration,debt,at,pips,fractional_tail);
    if(!before||before->first_zero)return {};
    const auto after=overcast::prepare_resize(before->curve,owner,before->resources,before->regeneration,
        before->debt,*next,at,pips,before->fractional_tail);
    if(!after)return {};
    return EnergyTransition{owner,at,held_before,held_after,std::move(*before),after->state};
}
}
