#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
namespace fw::gw::skills::casting {
// Opt-in adapter shared by future gateway integration and native tests. This
// does not enroll skill248 or manufacture an NPC spell-activation receipt.
// Owner supplies the current server-owned player scope, not a client actor ID.
template<class Owner>
auto make_retribution_completion(world::Instance& world,Owner owner){
    return [&world,owner=std::move(owner)](effects::Scope target,const effects::RetributionPlan& plan,
            unsigned divine,std::uint64_t event,std::uint64_t now)mutable->EffectCompletion{
        const auto source=owner();if(!source)return {};
        std::vector<combat::Packet> visuals;std::function<void()> publication;
        const auto done=world.complete_retribution(*source,target,plan,divine,now,event,&visuals,&publication);
        // Healthy World output is deferred behind Runtime58 and target454.
        // Consumed/lethal World output owns that prefix inside its closure.
        return {done.accepted,std::move(visuals),{},std::move(publication),true,done.completion_prefix_published};
    };
}
template<class Owner>
auto make_retribution_target_valid(world::Instance& world,Owner owner){
    return [&world,owner=std::move(owner)](effects::Scope target,unsigned divine,std::uint64_t now)mutable{
        const auto source=owner();return source&&world.retribution_target_valid(*source,target,divine,now);
    };
}
}
