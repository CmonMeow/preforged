#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
namespace fw::gw::skills::casting {
// Shared by the production gateway and native integration tests. The getter
// supplies the current server-owned caster scope, never an incoming actor ID.
template<class Owner>
auto make_life_steal_completion(world::Instance& world,Owner owner){
    return [&world,owner=std::move(owner)](effects::Scope target,const effects::LifeStealPlan& plan,
            std::uint64_t now)mutable->EffectCompletion{
        const auto source=owner();if(!source)return {};
        std::vector<combat::Packet> visuals;
        // World validates the complete canonical family. A second partial
        // ID list here previously rejected2189despite its Runtime producer.
        const bool accepted=world.life_steal(*source,target,plan,now,&visuals);
        return {accepted,std::move(visuals)};
    };
}
template<class Owner>
auto make_admitted_life_steal_completion(world::Instance& world,Owner owner){
    return [&world,owner=std::move(owner)](effects::Scope target,const effects::LifeStealPlan& plan,
            std::uint64_t event,std::uint64_t now)mutable->EffectCompletion{
        const auto source=owner();if(!source)return {};
        std::vector<combat::Packet> visuals;
        const bool accepted=world.complete_life_steal(*source,target,plan,event,now,&visuals);
        return {accepted,std::move(visuals)};
    };
}
}
