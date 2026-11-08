#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
namespace fw::gw::skills::casting {
// Shared staged adapter for the paid Runtime path and native tests.
// World validates the admitted event, awards pre-effect Renewal, and defers
// recipient publication until the Runtime publishes its completion prefix.
template<class Owner>
auto make_glimmering_mark_completion(world::Instance& world,Owner owner,unsigned level){
 return [&world,owner=std::move(owner),level](effects::Scope target,const effects::GlimmeringMarkPlan& plan,
        std::uint64_t event,std::uint64_t now)mutable->EffectCompletion{
  const auto source=owner();if(!source)return {};
  std::function<void()> publication;
  const bool accepted=world.complete_glimmering_mark(*source,target,plan,level,event,now,&publication);
  return {accepted,{}, {},std::move(publication)};
 };
}
}
