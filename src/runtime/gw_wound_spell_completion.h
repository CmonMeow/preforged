#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
namespace fw::gw::skills::casting {
// Same adapter in the gateway and native paid-cast tests. World owns the
// canonical plan, admitted event, recipient conditions and packet publication.
template<class Owner>
auto make_wound_spell_completion(world::Instance& world,Owner owner,unsigned level){
    return [&world,owner=std::move(owner),level](effects::Scope target,const effects::WoundSpellPlan& plan,
            std::uint64_t event,std::uint64_t now)mutable->EffectCompletion{
        const auto source=owner();if(!source)return {};
        std::vector<combat::Packet> visuals;std::function<void()> publication;
        const bool accepted=world.complete_wound_spell(*source,target,plan,level,event,now,&visuals,&publication);
        return {accepted,std::move(visuals),{},std::move(publication)};
    };
}
}
