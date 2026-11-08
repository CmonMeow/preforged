#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
namespace fw::gw::skills::casting {
// One adapter for production and paid native tests. World admits the event,
// pays pre-effect Renewal, and returns ground visuals for the finish prefix.
template<class Owner>
auto make_snow_storm_completion(world::Instance& world,Owner owner,unsigned level){
 return [&world,owner=std::move(owner),level](effects::Scope target,const effects::SnowStormPlan& plan,
        std::uint64_t event,std::uint64_t now)mutable->EffectCompletion{
  const auto source=owner();if(!source)return {};
  std::vector<world::Packet> visuals;
  const bool accepted=world.complete_snow_storm(*source,target,plan,level,event,now,&visuals);
  return {accepted,std::move(visuals)};
 };
}
}
