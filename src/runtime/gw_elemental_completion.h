#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
namespace fw::gw::skills::casting {
// Production and integration tests share the same admitted World consumer.
template<class Owner,class Level>
auto make_admitted_elemental_completion(world::Instance& world,Owner owner,Level level){
    return [&world,owner=std::move(owner),level=std::move(level)](effects::Scope target,const effects::ElementalPlan& plan,
            std::uint64_t event,std::uint64_t now)mutable->std::optional<ConditionalElementalCompletion>{
        const auto source=owner();if(!source)return {};
        effects::ElementalOutcome outcome;std::vector<combat::Packet> visuals;std::function<void()> publication;
        if(!world.elemental(*source,target,plan,level(),now,&visuals,nullptr,&outcome,event,&publication))return {};
        return ConditionalElementalCompletion{outcome,std::move(visuals),std::move(publication),true};
    };
}
}
