#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
namespace fw::gw::skills::casting {
template<class Owner>
auto make_defensive_ward_completion(world::Instance& world,Owner owner){
 return [&world,owner=std::move(owner)](const effects::DefensiveWardPlan& plan,
        std::uint64_t event,std::uint64_t now)mutable->EffectCompletion{
  const auto source=owner();if(!source)return {};
  std::vector<world::Packet> visuals;
  const bool accepted=world.complete_defensive_ward(*source,plan,event,now,&visuals);
  return {accepted,std::move(visuals)};
 };
}
}
